#include "test_host.h"

#include <string.h>

#include "http_config.h"
#include "http_response.h"

bool run_test_response()
{
    uint8_t buf[512];
    HttpResponse res;
    res.init(buf, sizeof(buf));
    CHECK_EQ_INT(res.send("hello"), HttpErr::OK);
    CHECK_TRUE(res.sent());
    CHECK_CONTAINS(reinterpret_cast<const char *>(res.data()), "HTTP/1.1 200 OK");
    CHECK_CONTAINS(reinterpret_cast<const char *>(res.data()), "Content-Type: text/plain");
    CHECK_CONTAINS(reinterpret_cast<const char *>(res.data()), "Content-Length: 5");
    CHECK_CONTAINS(reinterpret_cast<const char *>(res.data()), "Connection: close");
    CHECK_EQ_INT(res.send("again"), HttpErr::ALREADY_SENT);
    CHECK_EQ_INT(res.set_header("X", "y"), HttpErr::ALREADY_SENT);

    res.init(buf, sizeof(buf));
    CHECK_EQ_INT(res.send_json("{\"ok\":true}"), HttpErr::OK);
    CHECK_CONTAINS(reinterpret_cast<const char *>(res.data()), "application/json");

    res.init(buf, sizeof(buf));
    CHECK_EQ_INT(res.send_html("<h1>x</h1>"), HttpErr::OK);
    CHECK_CONTAINS(reinterpret_cast<const char *>(res.data()), "text/html; charset=utf-8");

    res.init(buf, sizeof(buf));
    CHECK_EQ_INT(res.end(204), HttpErr::OK);
    CHECK_CONTAINS(reinterpret_cast<const char *>(res.data()), "HTTP/1.1 204 No Content");
    CHECK_CONTAINS(reinterpret_cast<const char *>(res.data()), "Content-Length: 0");

    res.init(buf, sizeof(buf));
    for (int i = 0; i < HTTP_RESP_HEADER_MAX; i++) {
        char name[8];
        snprintf(name, sizeof(name), "X-%d", i);
        CHECK_EQ_INT(res.set_header(name, "v"), HttpErr::OK);
    }
    CHECK_EQ_INT(res.set_header("X-Full", "v"), HttpErr::HEADER_FULL);

    uint8_t body[HTTP_RESPONSE_BUFFER_SIZE + 1];
    int exact_body_len = -1;
    memset(body, 'x', sizeof(body));
    for (size_t body_len = 0; body_len <= HTTP_RESPONSE_BUFFER_SIZE; body_len++) {
        res.init(buf, sizeof(buf));
        if (res.send(body, body_len, "text/plain") == HttpErr::OK &&
            res.data_len() == HTTP_RESPONSE_BUFFER_SIZE) {
            exact_body_len = static_cast<int>(body_len);
            break;
        }
    }
    CHECK_TRUE(exact_body_len >= 0);

    res.init(buf, sizeof(buf));
    CHECK_EQ_INT(res.send(body,
                          static_cast<size_t>(exact_body_len) + 1,
                          "text/plain"),
                 HttpErr::SEND_FAILED);

    return true;
}
