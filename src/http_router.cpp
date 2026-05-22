#include "http_router.h"

#include <string.h>

bool http_route_match_exact(const char *pattern,
                            const char *path,
                            HttpRouteMatch *out_match)
{
    if (out_match != 0) {
        out_match->count = 0;
    }
    if (pattern == 0 || path == 0) {
        return false;
    }
    return strcmp(pattern, path) == 0;
}

HttpRouter::HttpRouter()
{
    count_ = 0;
}

HttpErr HttpRouter::add(HttpMethod method,
                        const char *path,
                        HttpHandler handler,
                        void *user_ctx,
                        HttpRouteMatcher matcher)
{
    if (path == 0 || path[0] == '\0' || handler == 0) {
        return HttpErr::INVALID_ARG;
    }
    if (count_ >= HTTP_MAX_ROUTES) {
        return HttpErr::ROUTE_FULL;
    }

    routes_[count_].method = method;
    routes_[count_].path = path;
    routes_[count_].handler = handler;
    routes_[count_].user_ctx = user_ctx;
    routes_[count_].matcher = matcher != 0 ? matcher : http_route_match_exact;
    count_++;
    return HttpErr::OK;
}

HttpRouteResult HttpRouter::resolve(HttpMethod method,
                                    const char *path,
                                    HttpResolvedRoute *out) const
{
    bool path_found = false;
    HttpRouteMatch match;

    if (out != 0) {
        out->route = 0;
        out->match.count = 0;
    }

    for (uint8_t i = 0; i < count_; i++) {
        match.count = 0;
        if (routes_[i].method == method && routes_[i].matcher(routes_[i].path, path, &match)) {
            if (out != 0) {
                out->route = &routes_[i];
                out->match = match;
            }
            return HttpRouteResult::FOUND;
        }
    }

    for (uint8_t i = 0; i < count_; i++) {
        match.count = 0;
        if (routes_[i].matcher(routes_[i].path, path, &match)) {
            path_found = true;
        }
    }

    return path_found ? HttpRouteResult::METHOD_NOT_ALLOWED : HttpRouteResult::NOT_FOUND;
}

uint8_t HttpRouter::count() const
{
    return count_;
}
