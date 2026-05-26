#ifndef HTTP_ENGINE_H
#define HTTP_ENGINE_H

#include <stddef.h>
#include <stdint.h>
#include "http_config.h"
#include "http_parser.h"
#include "http_response.h"
#include "http_router.h"
#if HTTP_ENABLE_STATIC_FILES
#include "http_static_files.h"
#endif

enum class HttpClientState : uint8_t {
    IDLE,
    READ_LINE,
    READ_HEADERS,
    READ_BODY,
    ROUTE,
    CALL_HANDLER,
    SEND_RESPONSE,
    SEND_FILE,
    CLOSING,
    ERROR
};

enum class HttpSendPhase : uint8_t {
    IDLE,
    HEADERS,
    BODY,
    DONE,
    ERROR
};

struct HttpClientSlot {
    bool used;
    void *conn;
    HttpClientState state;
    HttpParser parser;
    uint8_t tx[HTTP_RESPONSE_BUFFER_SIZE];
    size_t tx_len;
    bool close_after_send;
    HttpSendPhase send_phase;
    size_t body_len;
    size_t body_sent;
    const uint8_t *body_ram;
    const uint8_t *body_progmem;
    HttpBodyReader body_reader;
    void *body_reader_ctx;
#if HTTP_ENABLE_STATIC_FILES
    const HttpStaticRoute *static_route;
    void *static_handle;
    bool static_active;
#endif
};

class HttpEngine {
public:
    HttpEngine();

    void reset(uint8_t max_clients);
    HttpErr add_route(HttpMethod method,
                      const char *path,
                      HttpHandler handler,
                      void *user_ctx,
                      HttpRouteMatcher matcher);

#if HTTP_ENABLE_STATIC_FILES
    HttpErr add_static(const char *url_prefix,
                       const char *fs_root,
                       HttpFsBackend *backend,
                       const char *index_file);
#endif

    int alloc_slot(void *conn);
    void release_slot(int slot);
    int find_slot(void *conn) const;
    HttpErr on_data(int slot, const uint8_t *data, size_t len);
    HttpErr on_drain(int slot);
    void on_bytes_sent(int slot, size_t sent);
    const uint8_t *tx_data(int slot) const;
    size_t tx_len(int slot) const;
    bool close_after_send(int slot) const;
    bool ready_to_close(int slot) const;

    HttpRouter &router();

private:
    HttpErr dispatch(int slot);
    HttpErr send_error(int slot, uint16_t status, const char *text);
    HttpErr start_response(int slot, const HttpResponse &res);
#if HTTP_ENABLE_STATIC_FILES
    HttpErr start_static_response(int slot, const HttpStaticResolved &file);
#endif
    HttpErr fill_next_tx(int slot);
    HttpErr encode_headers(int slot, const HttpResponse &res);
    void close_static_if_needed(int slot);

    HttpClientSlot slots_[HTTP_MAX_CLIENTS];
    uint8_t max_clients_;
    HttpRouter router_;
#if HTTP_ENABLE_STATIC_FILES
    HttpStaticFiles static_files_;
#endif
};

#endif
