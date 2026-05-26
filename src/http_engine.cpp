#include "http_engine.h"

#include "http_common.h"

#include <stdio.h>
#include <string.h>

#if HTTP_DEBUG
#define HTTP_DBG(fmt, ...) printf("http_engine: " fmt "\n", ##__VA_ARGS__)
#else
#define HTTP_DBG(fmt, ...)
#endif

HttpEngine::HttpEngine()
{
    reset(HTTP_MAX_CLIENTS);
}

void HttpEngine::reset(uint8_t max_clients)
{
    if (max_clients == 0 || max_clients > HTTP_MAX_CLIENTS) {
        max_clients = HTTP_MAX_CLIENTS;
    }
    max_clients_ = max_clients;
    for (uint8_t i = 0; i < HTTP_MAX_CLIENTS; i++) {
        slots_[i].used = false;
        slots_[i].conn = 0;
        slots_[i].state = HttpClientState::IDLE;
        slots_[i].tx_len = 0;
        slots_[i].close_after_send = false;
        slots_[i].send_phase = HttpSendPhase::IDLE;
        slots_[i].body_len = 0;
        slots_[i].body_sent = 0;
        slots_[i].body_ram = 0;
        slots_[i].body_progmem = 0;
        slots_[i].body_reader = 0;
        slots_[i].body_reader_ctx = 0;
#if HTTP_ENABLE_STATIC_FILES
        slots_[i].static_route = 0;
        slots_[i].static_handle = 0;
        slots_[i].static_active = false;
#endif
        slots_[i].parser.reset();
    }
}

HttpErr HttpEngine::add_route(HttpMethod method,
                              const char *path,
                              HttpHandler handler,
                              void *user_ctx,
                              HttpRouteMatcher matcher)
{
    return router_.add(method, path, handler, user_ctx, matcher);
}

#if HTTP_ENABLE_STATIC_FILES
HttpErr HttpEngine::add_static(const char *url_prefix,
                               const char *fs_root,
                               HttpFsBackend *backend,
                               const char *index_file)
{
    return static_files_.add(url_prefix, fs_root, backend, index_file);
}
#endif

int HttpEngine::alloc_slot(void *conn)
{
    for (uint8_t i = 0; i < max_clients_; i++) {
        if (!slots_[i].used) {
            slots_[i].used = true;
            slots_[i].conn = conn;
            slots_[i].state = HttpClientState::READ_LINE;
            slots_[i].tx_len = 0;
            slots_[i].close_after_send = false;
            slots_[i].send_phase = HttpSendPhase::IDLE;
            slots_[i].body_len = 0;
            slots_[i].body_sent = 0;
            slots_[i].body_ram = 0;
            slots_[i].body_progmem = 0;
            slots_[i].body_reader = 0;
            slots_[i].body_reader_ctx = 0;
#if HTTP_ENABLE_STATIC_FILES
            slots_[i].static_route = 0;
            slots_[i].static_handle = 0;
            slots_[i].static_active = false;
#endif
            slots_[i].parser.reset();
            return i;
        }
    }
    return -1;
}

void HttpEngine::release_slot(int slot)
{
    if (slot < 0 || slot >= static_cast<int>(HTTP_MAX_CLIENTS)) {
        return;
    }
    close_static_if_needed(slot);
    slots_[slot].used = false;
    slots_[slot].conn = 0;
    slots_[slot].state = HttpClientState::IDLE;
    slots_[slot].tx_len = 0;
    slots_[slot].close_after_send = false;
    slots_[slot].send_phase = HttpSendPhase::IDLE;
    slots_[slot].body_len = 0;
    slots_[slot].body_sent = 0;
    slots_[slot].body_ram = 0;
    slots_[slot].body_progmem = 0;
    slots_[slot].body_reader = 0;
    slots_[slot].body_reader_ctx = 0;
#if HTTP_ENABLE_STATIC_FILES
    slots_[slot].static_route = 0;
    slots_[slot].static_handle = 0;
    slots_[slot].static_active = false;
#endif
    slots_[slot].parser.reset();
}

void HttpEngine::close_static_if_needed(int slot)
{
#if HTTP_ENABLE_STATIC_FILES
    if (slot < 0 || slot >= static_cast<int>(HTTP_MAX_CLIENTS)) {
        return;
    }
    if (slots_[slot].static_active &&
        slots_[slot].static_route != 0 &&
        slots_[slot].static_route->backend != 0 &&
        slots_[slot].static_route->backend->close != 0 &&
        slots_[slot].static_handle != 0) {
        slots_[slot].static_route->backend->close(slots_[slot].static_route->backend->ctx,
                                                  slots_[slot].static_handle);
    }
    slots_[slot].static_active = false;
    slots_[slot].static_route = 0;
    slots_[slot].static_handle = 0;
#else
    (void)slot;
#endif
}

int HttpEngine::find_slot(void *conn) const
{
    for (uint8_t i = 0; i < max_clients_; i++) {
        if (slots_[i].used && slots_[i].conn == conn) {
            return i;
        }
    }
    return -1;
}

HttpErr HttpEngine::send_error(int slot, uint16_t status, const char *text)
{
    HTTP_DBG("send_error slot=%d status=%u", slot, (unsigned)status);
    HttpResponse res;
    res.init(slots_[slot].tx, sizeof(slots_[slot].tx));
    res.set_status(status);
    HttpErr err = res.send(text);
    if (err != HttpErr::OK) {
        slots_[slot].state = HttpClientState::ERROR;
        return err;
    }
    err = start_response(slot, res);
    if (err != HttpErr::OK) {
        slots_[slot].state = HttpClientState::ERROR;
        return err;
    }
    slots_[slot].state = HttpClientState::CLOSING;
    return HttpErr::OK;
}

HttpErr HttpEngine::encode_headers(int slot, const HttpResponse &res)
{
    size_t n;
    int r = snprintf(reinterpret_cast<char *>(slots_[slot].tx),
                     sizeof(slots_[slot].tx),
                     "HTTP/1.1 %u %s\r\n",
                     static_cast<unsigned>(res.status()),
                     http_status_reason(res.status()));
    if (r <= 0 || static_cast<size_t>(r) >= sizeof(slots_[slot].tx)) {
        return HttpErr::SEND_FAILED;
    }
    n = static_cast<size_t>(r);

    bool has_content_type = false;
    for (uint8_t i = 0; i < res.header_count(); i++) {
        const char *name = res.header_name(i);
        const char *value = res.header_value(i);
        if (name == 0 || value == 0) {
            return HttpErr::SEND_FAILED;
        }
        if (http_str_case_equal(name, "Content-Type")) {
            has_content_type = true;
        }
        r = snprintf(reinterpret_cast<char *>(slots_[slot].tx + n),
                     sizeof(slots_[slot].tx) - n,
                     "%s: %s\r\n",
                     name,
                     value);
        if (r <= 0 || static_cast<size_t>(r) >= (sizeof(slots_[slot].tx) - n)) {
            return HttpErr::SEND_FAILED;
        }
        n += static_cast<size_t>(r);
    }

    if (!has_content_type && res.content_type() != 0 && res.content_type()[0] != '\0') {
        r = snprintf(reinterpret_cast<char *>(slots_[slot].tx + n),
                     sizeof(slots_[slot].tx) - n,
                     "Content-Type: %s\r\n",
                     res.content_type());
        if (r <= 0 || static_cast<size_t>(r) >= (sizeof(slots_[slot].tx) - n)) {
            return HttpErr::SEND_FAILED;
        }
        n += static_cast<size_t>(r);
    }

    r = snprintf(reinterpret_cast<char *>(slots_[slot].tx + n),
                 sizeof(slots_[slot].tx) - n,
                 "Content-Length: %lu\r\nConnection: close\r\n\r\n",
                 static_cast<unsigned long>(res.body_length()));
    if (r <= 0 || static_cast<size_t>(r) >= (sizeof(slots_[slot].tx) - n)) {
        return HttpErr::SEND_FAILED;
    }
    n += static_cast<size_t>(r);

    slots_[slot].tx_len = n;
    return HttpErr::OK;
}

HttpErr HttpEngine::start_response(int slot, const HttpResponse &res)
{
    close_static_if_needed(slot);
    HttpErr err = encode_headers(slot, res);
    if (err != HttpErr::OK) {
        return err;
    }

    slots_[slot].send_phase = HttpSendPhase::HEADERS;
    slots_[slot].close_after_send = true;
    slots_[slot].body_len = res.body_length();
    slots_[slot].body_sent = 0;
    slots_[slot].body_ram = res.body_ram();
    slots_[slot].body_progmem = res.body_progmem();
    slots_[slot].body_reader = res.body_reader();
    slots_[slot].body_reader_ctx = res.body_reader_ctx();
    if (res.body_kind() == HttpBodyKind::NONE || res.body_length() == 0) {
        slots_[slot].send_phase = HttpSendPhase::DONE;
    } else {
        slots_[slot].send_phase = HttpSendPhase::HEADERS;
    }
    return HttpErr::OK;
}

#if HTTP_ENABLE_STATIC_FILES
HttpErr HttpEngine::start_static_response(int slot, const HttpStaticResolved &file)
{
    int header_len;
    close_static_if_needed(slot);
    header_len = snprintf(reinterpret_cast<char *>(slots_[slot].tx),
                          sizeof(slots_[slot].tx),
                          "HTTP/1.1 200 OK\r\n"
                          "Content-Type: %s\r\n"
                          "Content-Length: %lu\r\n"
                          "Connection: close\r\n\r\n",
                          file.mime,
                          static_cast<unsigned long>(file.size));
    if (header_len <= 0 || static_cast<size_t>(header_len) >= sizeof(slots_[slot].tx)) {
        return HttpErr::SEND_FAILED;
    }

    slots_[slot].tx_len = static_cast<size_t>(header_len);
    slots_[slot].send_phase = HttpSendPhase::HEADERS;
    slots_[slot].close_after_send = true;
    slots_[slot].body_len = file.size;
    slots_[slot].body_sent = 0;
    slots_[slot].body_ram = 0;
    slots_[slot].body_progmem = 0;
    slots_[slot].body_reader = 0;
    slots_[slot].body_reader_ctx = 0;
    slots_[slot].static_route = file.route;
    slots_[slot].static_handle = file.handle;
    slots_[slot].static_active = true;
    return HttpErr::OK;
}
#endif

HttpErr HttpEngine::fill_next_tx(int slot)
{
    if (slots_[slot].tx_len != 0) {
        return HttpErr::OK;
    }
    if (slots_[slot].send_phase == HttpSendPhase::DONE ||
        slots_[slot].send_phase == HttpSendPhase::IDLE ||
        slots_[slot].send_phase == HttpSendPhase::ERROR) {
        return HttpErr::OK;
    }
    if (slots_[slot].send_phase == HttpSendPhase::HEADERS) {
        if (slots_[slot].body_len == 0) {
            slots_[slot].send_phase = HttpSendPhase::DONE;
            return HttpErr::OK;
        }
        slots_[slot].send_phase = HttpSendPhase::BODY;
    }

    if (slots_[slot].send_phase != HttpSendPhase::BODY) {
        return HttpErr::OK;
    }

    size_t remaining = slots_[slot].body_len - slots_[slot].body_sent;
    size_t chunk = remaining < sizeof(slots_[slot].tx) ? remaining : sizeof(slots_[slot].tx);
    if (chunk == 0) {
        slots_[slot].send_phase = HttpSendPhase::DONE;
        return HttpErr::OK;
    }

    if (slots_[slot].body_ram != 0) {
        memcpy(slots_[slot].tx,
               slots_[slot].body_ram + slots_[slot].body_sent,
               chunk);
        slots_[slot].tx_len = chunk;
        return HttpErr::OK;
    }
    if (slots_[slot].body_progmem != 0) {
        memcpy(slots_[slot].tx,
               slots_[slot].body_progmem + slots_[slot].body_sent,
               chunk);
        slots_[slot].tx_len = chunk;
        return HttpErr::OK;
    }
    if (slots_[slot].body_reader != 0) {
        size_t got = slots_[slot].body_reader(slots_[slot].body_reader_ctx,
                                              slots_[slot].body_sent,
                                              slots_[slot].tx,
                                              chunk);
        if (got == 0 || got > chunk) {
            slots_[slot].send_phase = HttpSendPhase::ERROR;
            return HttpErr::SEND_FAILED;
        }
        slots_[slot].tx_len = got;
        return HttpErr::OK;
    }
#if HTTP_ENABLE_STATIC_FILES
    if (slots_[slot].static_active &&
        slots_[slot].static_route != 0 &&
        slots_[slot].static_route->backend != 0 &&
        slots_[slot].static_route->backend->read != 0 &&
        slots_[slot].static_handle != 0) {
        size_t got = slots_[slot].static_route->backend->read(slots_[slot].static_route->backend->ctx,
                                                              slots_[slot].static_handle,
                                                              slots_[slot].tx,
                                                              chunk);
        if (got == 0 || got > chunk) {
            slots_[slot].send_phase = HttpSendPhase::ERROR;
            close_static_if_needed(slot);
            return HttpErr::FS_ERROR;
        }
        slots_[slot].tx_len = got;
        return HttpErr::OK;
    }
#endif
    slots_[slot].send_phase = HttpSendPhase::ERROR;
    return HttpErr::SEND_FAILED;
}

HttpErr HttpEngine::dispatch(int slot)
{
    HttpResolvedRoute resolved;
    HttpRouteResult rr;
    HttpRequest req;
    HttpResponse res;

    slots_[slot].state = HttpClientState::ROUTE;
    rr = router_.resolve(slots_[slot].parser.method(), slots_[slot].parser.path(), &resolved);
    HTTP_DBG("route slot=%d method=%s path=%s result=%u",
             slot,
             http_method_name(slots_[slot].parser.method()),
             slots_[slot].parser.path(),
             (unsigned)rr);
    if (rr == HttpRouteResult::METHOD_NOT_ALLOWED) {
        return send_error(slot, 405, "Method Not Allowed");
    }
    if (rr == HttpRouteResult::NOT_FOUND) {
#if HTTP_ENABLE_STATIC_FILES
        HttpStaticResolved file;
        if (slots_[slot].parser.method() == HttpMethod::GET &&
            static_files_.resolve_open(slots_[slot].parser.path(), &file)) {
            HttpErr se = start_static_response(slot, file);
            if (se != HttpErr::OK) {
                file.route->backend->close(file.route->backend->ctx, file.handle);
                return send_error(slot, 500, "Internal Server Error");
            }
            slots_[slot].state = HttpClientState::CLOSING;
            return HttpErr::OK;
        }
#endif
        return send_error(slot, 404, "Not Found");
    }

    /* HttpRouter guarantees this today; keep the guard to preserve the invariant. */
    if (resolved.route == 0 || resolved.route->handler == 0) {
        return send_error(slot, 500, "Internal Server Error");
    }

    slots_[slot].state = HttpClientState::CALL_HANDLER;
    req.init(slots_[slot].parser.method(),
             slots_[slot].parser.path(),
             slots_[slot].parser.query_raw(),
             slots_[slot].parser.headers(),
             slots_[slot].parser.header_count(),
             slots_[slot].parser.body(),
             slots_[slot].parser.body_len(),
             resolved.route->user_ctx,
             &resolved.match);
    res.init(slots_[slot].tx, sizeof(slots_[slot].tx));
    resolved.route->handler(req, res);
    if (!res.sent()) {
        return send_error(slot, 500, "Internal Server Error");
    }
    HttpErr enc = start_response(slot, res);
    if (enc != HttpErr::OK) {
        return send_error(slot, 500, "Internal Server Error");
    }
    slots_[slot].state = HttpClientState::CLOSING;
    return HttpErr::OK;
}

HttpErr HttpEngine::on_data(int slot, const uint8_t *data, size_t len)
{
    if (slot < 0 || slot >= static_cast<int>(HTTP_MAX_CLIENTS) || !slots_[slot].used) {
        return HttpErr::INVALID_ARG;
    }

    HttpParserResult r = slots_[slot].parser.feed(data, len);
    if (r == HttpParserResult::BODY_TOO_LARGE) {
        HTTP_DBG("parse body too large slot=%d", slot);
        return send_error(slot, 413, "Payload Too Large");
    }
    if (r == HttpParserResult::ERROR) {
        HTTP_DBG("parse error slot=%d", slot);
        HttpErr err = send_error(slot, 400, "Bad Request");
        return err == HttpErr::OK ? HttpErr::PARSE_ERROR : err;
    }
    if (r == HttpParserResult::DONE) {
        HTTP_DBG("parse done slot=%d", slot);
        return dispatch(slot);
    }
    return HttpErr::OK;
}

HttpErr HttpEngine::on_drain(int slot)
{
    if (slot < 0 || slot >= static_cast<int>(HTTP_MAX_CLIENTS) || !slots_[slot].used) {
        return HttpErr::INVALID_ARG;
    }
    return fill_next_tx(slot);
}

void HttpEngine::on_bytes_sent(int slot, size_t sent)
{
    if (slot < 0 || slot >= static_cast<int>(HTTP_MAX_CLIENTS) || !slots_[slot].used || sent == 0) {
        return;
    }
    if (sent >= slots_[slot].tx_len) {
        size_t consumed = slots_[slot].tx_len;
        slots_[slot].tx_len = 0;
        if (slots_[slot].send_phase == HttpSendPhase::BODY) {
            slots_[slot].body_sent += consumed;
            if (slots_[slot].body_sent >= slots_[slot].body_len) {
                slots_[slot].send_phase = HttpSendPhase::DONE;
                close_static_if_needed(slot);
            }
        } else if (slots_[slot].send_phase == HttpSendPhase::HEADERS) {
            if (slots_[slot].body_len == 0) {
                slots_[slot].send_phase = HttpSendPhase::DONE;
            } else {
                slots_[slot].send_phase = HttpSendPhase::BODY;
            }
        }
        return;
    }
    memmove(slots_[slot].tx, slots_[slot].tx + sent, slots_[slot].tx_len - sent);
    slots_[slot].tx_len -= sent;
}

const uint8_t *HttpEngine::tx_data(int slot) const
{
    return slots_[slot].tx;
}

size_t HttpEngine::tx_len(int slot) const
{
    return slots_[slot].tx_len;
}

bool HttpEngine::close_after_send(int slot) const
{
    return slots_[slot].close_after_send;
}

bool HttpEngine::ready_to_close(int slot) const
{
    return slots_[slot].close_after_send &&
           slots_[slot].tx_len == 0 &&
           slots_[slot].send_phase == HttpSendPhase::DONE;
}

HttpRouter &HttpEngine::router()
{
    return router_;
}
