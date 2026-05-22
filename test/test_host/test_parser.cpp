#include "test_host.h"

#include "http_parser.h"

static HttpParserResult feed_text(HttpParser &p, const char *s)
{
    return p.feed(reinterpret_cast<const uint8_t *>(s), strlen(s));
}

bool run_test_parser()
{
    HttpParser p;
    CHECK_EQ_INT(feed_text(p, "GET / HTTP/1.1\r\nHost: x\r\n\r\n"), HttpParserResult::DONE);
    CHECK_EQ_INT(p.method(), HttpMethod::GET);
    CHECK_STR(p.path(), "/");
    CHECK_STR(p.header("host"), "x");

    p.reset();
    CHECK_EQ_INT(feed_text(p, "GET /api/status?x=1 HTTP/1.1\r\n\r\n"), HttpParserResult::DONE);
    CHECK_STR(p.path(), "/api/status");
    CHECK_STR(p.query_raw(), "x=1");

    p.reset();
    CHECK_EQ_INT(feed_text(p, "POST /api/config HTTP/1.1\r\nContent-Length: 7\r\nContent-Type: application/json\r\n\r\n{\"a\":1}"), HttpParserResult::DONE);
    CHECK_EQ_INT(p.method(), HttpMethod::POST);
    CHECK_EQ_INT(p.body_len(), 7);

    p.reset();
    CHECK_EQ_INT(feed_text(p, "PATCH /x HTTP/1.1\r\n\r\n"), HttpParserResult::DONE);
    CHECK_EQ_INT(p.method(), HttpMethod::UNKNOWN);

    p.reset();
    CHECK_EQ_INT(feed_text(p, "GET /x HTTP/1.1\r\nContent-Length: nope\r\n\r\n"), HttpParserResult::ERROR);

    p.reset();
    CHECK_EQ_INT(feed_text(p, "GET /x HTTP/1.1\r\nContent-Length: 2048\r\n\r\n"), HttpParserResult::BODY_TOO_LARGE);

    p.reset();
    CHECK_EQ_INT(feed_text(p, "POST /x HTTP/1.1\r\nContent-Length: 4\r\n\r\nab"), HttpParserResult::NEED_MORE);
    CHECK_EQ_INT(feed_text(p, "cd"), HttpParserResult::DONE);
    CHECK_EQ_INT(p.body_len(), 4);

    return true;
}
