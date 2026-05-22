#include "http_server.h"

#include "http_engine.h"
#include "http_tcp_adapter.h"

static HttpEngine g_http_engine;

HttpServer::HttpServer(uint16_t port, uint8_t max_clients)
    : port_(port),
      max_clients_(max_clients),
      started_(false),
      impl_(&g_http_engine)
{
    g_http_engine.reset(max_clients_);
}

HttpErr HttpServer::on(HttpMethod method,
                       const char *path,
                       HttpHandler handler,
                       void *user_ctx,
                       HttpRouteMatcher matcher)
{
    (void)impl_;
    return g_http_engine.add_route(method, path, handler, user_ctx, matcher);
}

#if HTTP_ENABLE_STATIC_FILES
HttpErr HttpServer::serve_static(const char *url_prefix,
                                 const char *fs_root,
                                 HttpFsBackend *backend,
                                 const char *index_file)
{
    return g_http_engine.add_static(url_prefix, fs_root, backend, index_file);
}
#endif

HttpErr HttpServer::begin()
{
    if (started_) {
        return HttpErr::OK;
    }
    HttpErr err = http_tcp_adapter_start(port_, max_clients_, &g_http_engine);
    if (err == HttpErr::OK) {
        started_ = true;
    }
    return err;
}

void HttpServer::stop()
{
    if (!started_) {
        return;
    }
    http_tcp_adapter_stop();
    g_http_engine.reset(max_clients_);
    started_ = false;
}
