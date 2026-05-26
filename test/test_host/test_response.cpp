#include "test_host.h"

#include <string.h>

#include "http_config.h"
#include "http_response.h"

static size_t stream_reader(void *ctx, size_t offset, uint8_t *dst, size_t max_len)
{
    const char *text = reinterpret_cast<const char *>(ctx);
    size_t total = strlen(text);
    size_t remaining = total - offset;
    size_t n = remaining < max_len ? remaining : max_len;
    memcpy(dst, text + offset, n);
    return n;
}

bool run_test_response()
{
    uint8_t buf[512];
    HttpResponse res;
    res.init(buf, sizeof(buf));
    CHECK_EQ_INT(res.send("hello"), HttpErr::OK);
    CHECK_TRUE(res.sent());
    CHECK_EQ_INT(res.status(), 200);
    CHECK_EQ_INT(static_cast<int>(res.body_kind()), static_cast<int>(HttpBodyKind::RAM));
    CHECK_EQ_INT(static_cast<int>(res.body_length()), 5);
    CHECK_CONTAINS(res.content_type(), "text/plain");
    CHECK_EQ_INT(res.send("again"), HttpErr::ALREADY_SENT);
    CHECK_EQ_INT(res.set_header("X", "y"), HttpErr::ALREADY_SENT);

    res.init(buf, sizeof(buf));
    CHECK_EQ_INT(res.send_json("{\"ok\":true}"), HttpErr::OK);
    CHECK_CONTAINS(res.content_type(), "application/json");

    res.init(buf, sizeof(buf));
    CHECK_EQ_INT(res.send_html("<h1>x</h1>"), HttpErr::OK);
    CHECK_CONTAINS(res.content_type(), "text/html; charset=utf-8");

    res.init(buf, sizeof(buf));
    CHECK_EQ_INT(res.send_progmem(reinterpret_cast<const uint8_t *>("<p>p</p>"),
                                  strlen("<p>p</p>"),
                                  "text/html; charset=utf-8"),
                 HttpErr::OK);
    CHECK_EQ_INT(static_cast<int>(res.body_kind()), static_cast<int>(HttpBodyKind::PROGMEM));
    CHECK_EQ_INT(static_cast<int>(res.body_length()), static_cast<int>(strlen("<p>p</p>")));

    res.init(buf, sizeof(buf));
    const char *stream_text = "stream-body";
    CHECK_EQ_INT(res.send_stream(stream_reader,
                                 const_cast<char *>(stream_text),
                                 strlen(stream_text),
                                 "text/plain"),
                 HttpErr::OK);
    CHECK_EQ_INT(static_cast<int>(res.body_kind()), static_cast<int>(HttpBodyKind::CALLBACK));
    CHECK_EQ_INT(static_cast<int>(res.body_length()), static_cast<int>(strlen(stream_text)));
    CHECK_TRUE(res.body_reader() != 0);

    res.init(buf, sizeof(buf));
    CHECK_EQ_INT(res.end(204), HttpErr::OK);
    CHECK_EQ_INT(res.status(), 204);
    CHECK_EQ_INT(static_cast<int>(res.body_length()), 0);
    CHECK_EQ_INT(static_cast<int>(res.body_kind()), static_cast<int>(HttpBodyKind::NONE));

    res.init(buf, sizeof(buf));
    for (int i = 0; i < HTTP_RESP_HEADER_MAX; i++) {
        char name[8];
        snprintf(name, sizeof(name), "X-%d", i);
        CHECK_EQ_INT(res.set_header(name, "v"), HttpErr::OK);
    }
    CHECK_EQ_INT(res.set_header("X-Full", "v"), HttpErr::HEADER_FULL);

    uint8_t body[HTTP_RESPONSE_BUFFER_SIZE + 1];
    memset(body, 'x', sizeof(body));
    res.init(buf, sizeof(buf));
    CHECK_EQ_INT(res.send(body,
                          HTTP_RESPONSE_BUFFER_SIZE + 1,
                          "text/plain"),
                 HttpErr::OK);
    CHECK_EQ_INT(static_cast<int>(res.body_length()), static_cast<int>(HTTP_RESPONSE_BUFFER_SIZE + 1));

    return true;
}
