#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <stdint.h>
#include "http_config.h"
#include "http_types.h"
#include "http_request.h"
#include "http_response.h"
#if HTTP_ENABLE_STATIC_FILES
#include "http_fs_backend.h"
#endif

class HttpServer {
public:
    explicit HttpServer(uint16_t port, uint8_t max_clients = HTTP_MAX_CLIENTS);

    HttpErr on(HttpMethod method,
               const char *path,
               HttpHandler handler,
               void *user_ctx = 0,
               HttpRouteMatcher matcher = 0);

#if HTTP_ENABLE_STATIC_FILES
    HttpErr serve_static(const char *url_prefix,
                         const char *fs_root,
                         HttpFsBackend *backend,
                         const char *index_file = "index.html");
#endif

    HttpErr begin();
    void stop();

private:
    uint16_t port_;
    uint8_t max_clients_;
    bool started_;
    void *impl_;
};

#endif
