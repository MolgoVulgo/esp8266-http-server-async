#include "test_host.h"

#include "http_request.h"

bool run_test_request()
{
    HttpParsedHeaderView headers[2];
    const uint8_t body[] = "name=test+value&empty=";
    HttpRequest req;
    char out[32];

    headers[0].name = "Content-Type";
    headers[0].value = "application/x-www-form-urlencoded";
    headers[1].name = "X-Test";
    headers[1].value = "ok";
    req.init(HttpMethod::POST, "/submit", "a=1&b=hello%20world", headers, 2,
             body, sizeof(body) - 1, reinterpret_cast<void *>(0x1234), 0);

    CHECK_STR(req.header("x-test"), "ok");
    CHECK_TRUE(req.header("missing") == 0);
    CHECK_TRUE(req.param("b", out, sizeof(out)));
    CHECK_STR(out, "hello world");
    CHECK_TRUE(!req.param("missing", out, sizeof(out)));
    CHECK_TRUE(!req.param("b", out, 4));
    CHECK_TRUE(req.form_param("name", out, sizeof(out)));
    CHECK_STR(out, "test value");
    CHECK_TRUE(!req.route_param("id", out, sizeof(out)));

    headers[0].value = "application/json";
    CHECK_TRUE(!req.form_param("name", out, sizeof(out)));
    return true;
}
