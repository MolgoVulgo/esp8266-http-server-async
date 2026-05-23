#include "http_engine.h"

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
    slots_[slot].used = false;
    slots_[slot].conn = 0;
    slots_[slot].state = HttpClientState::IDLE;
    slots_[slot].tx_len = 0;
    slots_[slot].close_after_send = false;
    slots_[slot].parser.reset();
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
    slots_[slot].tx_len = res.data_len();
    slots_[slot].close_after_send = true;
    slots_[slot].state = HttpClientState::CLOSING;
    return HttpErr::OK;
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
            int header_len = snprintf(reinterpret_cast<char *>(slots_[slot].tx),
                                      sizeof(slots_[slot].tx),
                                      "HTTP/1.1 200 OK\r\n"
                                      "Content-Type: %s\r\n"
                                      "Content-Length: %lu\r\n"
                                      "Connection: close\r\n\r\n",
                                      file.mime,
                                      static_cast<unsigned long>(file.size));
            if (header_len > 0 && static_cast<size_t>(header_len) < sizeof(slots_[slot].tx)) {
                size_t offset = static_cast<size_t>(header_len);
                size_t body_read = 0;
                if (file.size > sizeof(slots_[slot].tx) - offset) {
                    file.route->backend->close(file.route->backend->ctx, file.handle);
                    return send_error(slot, 500, "Internal Server Error");
                }
                while (body_read < file.size) {
                    size_t remaining = file.size - body_read;
                    size_t available = sizeof(slots_[slot].tx) - offset;
                    size_t to_read = remaining < available ? remaining : available;
                    size_t n = file.route->backend->read(file.route->backend->ctx,
                                                          file.handle,
                                                          slots_[slot].tx + offset,
                                                          to_read);
                    if (n == 0) {
                        break;
                    }
                    if (n > to_read) {
                        file.route->backend->close(file.route->backend->ctx, file.handle);
                        return send_error(slot, 500, "Internal Server Error");
                    }
                    offset += n;
                    body_read += n;
                }
                if (body_read != file.size) {
                    file.route->backend->close(file.route->backend->ctx, file.handle);
                    return send_error(slot, 500, "Internal Server Error");
                }
                slots_[slot].tx_len = offset;
                slots_[slot].close_after_send = true;
                slots_[slot].state = HttpClientState::CLOSING;
                file.route->backend->close(file.route->backend->ctx, file.handle);
                return HttpErr::OK;
            }
            file.route->backend->close(file.route->backend->ctx, file.handle);
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
    slots_[slot].tx_len = res.data_len();
    slots_[slot].close_after_send = true;
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
        return send_error(slot, 400, "Bad Request");
    }
    if (r == HttpParserResult::DONE) {
        HTTP_DBG("parse done slot=%d", slot);
        return dispatch(slot);
    }
    return HttpErr::OK;
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

HttpRouter &HttpEngine::router()
{
    return router_;
}
