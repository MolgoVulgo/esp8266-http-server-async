#ifndef HTTP_ROUTER_H
#define HTTP_ROUTER_H

#include "http_config.h"
#include "http_types.h"

struct HttpRouteEntry {
    HttpMethod method;
    const char *path;
    HttpHandler handler;
    void *user_ctx;
    HttpRouteMatcher matcher;
};

enum class HttpRouteResult : uint8_t {
    FOUND,
    METHOD_NOT_ALLOWED,
    NOT_FOUND
};

struct HttpResolvedRoute {
    const HttpRouteEntry *route;
    HttpRouteMatch match;
};

class HttpRouter {
public:
    HttpRouter();

    HttpErr add(HttpMethod method,
                const char *path,
                HttpHandler handler,
                void *user_ctx,
                HttpRouteMatcher matcher);

    HttpRouteResult resolve(HttpMethod method,
                            const char *path,
                            HttpResolvedRoute *out) const;

    uint8_t count() const;

private:
    HttpRouteEntry routes_[HTTP_MAX_ROUTES];
    uint8_t count_;
};

#endif
