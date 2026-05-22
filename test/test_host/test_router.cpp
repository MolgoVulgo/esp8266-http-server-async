#include "test_host.h"

#include "http_router.h"

static bool handler_called;

static void route_handler(HttpRequest &, HttpResponse &)
{
    handler_called = true;
}

static bool prefix_match(const char *pattern, const char *path, HttpRouteMatch *out)
{
    if (out != 0) {
        out->count = 0;
    }
    return strncmp(path, pattern, strlen(pattern)) == 0;
}

bool run_test_router()
{
    HttpRouter router;
    HttpResolvedRoute out;

    handler_called = false;
    CHECK_EQ_INT(router.add(HttpMethod::GET, "/a", route_handler, 0, 0), HttpErr::OK);
    CHECK_EQ_INT(router.resolve(HttpMethod::GET, "/a", &out), HttpRouteResult::FOUND);
    CHECK_TRUE(out.route->handler == route_handler);
    CHECK_EQ_INT(router.resolve(HttpMethod::POST, "/a", &out), HttpRouteResult::METHOD_NOT_ALLOWED);
    CHECK_EQ_INT(router.resolve(HttpMethod::GET, "/missing", &out), HttpRouteResult::NOT_FOUND);
    CHECK_EQ_INT(router.add(HttpMethod::GET, "/api", route_handler, 0, prefix_match), HttpErr::OK);
    CHECK_EQ_INT(router.resolve(HttpMethod::GET, "/api/status", &out), HttpRouteResult::FOUND);
    CHECK_EQ_INT(router.add(HttpMethod::GET, "", route_handler, 0, 0), HttpErr::INVALID_ARG);
    CHECK_EQ_INT(router.add(HttpMethod::GET, "/bad", 0, 0, 0), HttpErr::INVALID_ARG);

    for (int i = router.count(); i < HTTP_MAX_ROUTES; i++) {
        CHECK_EQ_INT(router.add(HttpMethod::GET, "/x", route_handler, 0, 0), HttpErr::OK);
    }
    CHECK_EQ_INT(router.add(HttpMethod::GET, "/overflow", route_handler, 0, 0), HttpErr::ROUTE_FULL);

    return true;
}
