#include "test_host.h"

#include <string.h>

#include "http_transport_mock.h"

static void ok_handler(HttpRequest &, HttpResponse &res)
{
    res.send("ok");
}

static void large_ram_handler(HttpRequest &, HttpResponse &res)
{
    static char body[HTTP_RESPONSE_BUFFER_SIZE * 2 + 1];
    for (size_t i = 0; i < sizeof(body) - 1; i++) {
        body[i] = 'A' + static_cast<char>(i % 26);
    }
    body[sizeof(body) - 1] = '\0';
    res.send(reinterpret_cast<const uint8_t *>(body), sizeof(body) - 1, "text/plain");
}

static size_t stream_reader(void *ctx, size_t offset, uint8_t *dst, size_t max_len)
{
    const char *text = reinterpret_cast<const char *>(ctx);
    size_t total = strlen(text);
    size_t remaining = total - offset;
    size_t n = remaining < max_len ? remaining : max_len;
    memcpy(dst, text + offset, n);
    return n;
}

static void stream_handler(HttpRequest &, HttpResponse &res)
{
    static char body[HTTP_RESPONSE_BUFFER_SIZE + 200];
    for (size_t i = 0; i < sizeof(body); i++) {
        body[i] = '0' + static_cast<char>(i % 10);
    }
    res.send_stream(stream_reader, body, sizeof(body), "text/plain");
}

static void no_response_handler(HttpRequest &, HttpResponse &)
{
}

bool run_test_engine()
{
    HttpTransportMock mock;
    http_transport_mock_init(&mock);
    CHECK_EQ_INT(mock.engine.add_route(HttpMethod::GET, "/", ok_handler, 0, 0), HttpErr::OK);
    CHECK_EQ_INT(http_transport_mock_rx(&mock, "GET / HTTP/1.1\r\n\r\n"), HttpErr::OK);
    CHECK_CONTAINS(http_transport_mock_tx(&mock), "HTTP/1.1 200 OK");
    CHECK_CONTAINS(http_transport_mock_tx(&mock), "\r\n\r\nok");
    CHECK_TRUE(mock.engine.close_after_send(mock.slot));
    http_transport_mock_close(&mock);

    http_transport_mock_init(&mock);
    CHECK_EQ_INT(mock.engine.add_route(HttpMethod::GET, "/", ok_handler, 0, 0), HttpErr::OK);
    CHECK_EQ_INT(http_transport_mock_rx(&mock, "GET /missing HTTP/1.1\r\n\r\n"), HttpErr::OK);
    CHECK_CONTAINS(http_transport_mock_tx(&mock), "HTTP/1.1 404 Not Found");
    http_transport_mock_close(&mock);

    http_transport_mock_init(&mock);
    CHECK_EQ_INT(mock.engine.add_route(HttpMethod::GET, "/", ok_handler, 0, 0), HttpErr::OK);
    CHECK_EQ_INT(http_transport_mock_rx(&mock, "POST / HTTP/1.1\r\n\r\n"), HttpErr::OK);
    CHECK_CONTAINS(http_transport_mock_tx(&mock), "HTTP/1.1 405 Method Not Allowed");
    http_transport_mock_close(&mock);

    http_transport_mock_init(&mock);
    CHECK_EQ_INT(mock.engine.add_route(HttpMethod::GET, "/", no_response_handler, 0, 0), HttpErr::OK);
    CHECK_EQ_INT(http_transport_mock_rx(&mock, "GET / HTTP/1.1\r\n\r\n"), HttpErr::OK);
    CHECK_CONTAINS(http_transport_mock_tx(&mock), "HTTP/1.1 500 Internal Server Error");
    http_transport_mock_close(&mock);

    http_transport_mock_init(&mock);
    CHECK_EQ_INT(http_transport_mock_rx(&mock, "GET /bad HTTP/1.1\r\nContent-Length: nope\r\n\r\n"), HttpErr::PARSE_ERROR);
    CHECK_CONTAINS(http_transport_mock_tx(&mock), "HTTP/1.1 400 Bad Request");
    http_transport_mock_close(&mock);

    http_transport_mock_init(&mock);
    CHECK_EQ_INT(http_transport_mock_rx(&mock, "POST /x HTTP/1.1\r\nContent-Length: 2048\r\n\r\n"), HttpErr::OK);
    CHECK_CONTAINS(http_transport_mock_tx(&mock), "HTTP/1.1 413 Payload Too Large");
    http_transport_mock_close(&mock);

    http_transport_mock_init(&mock);
    CHECK_EQ_INT(mock.engine.add_route(HttpMethod::POST, "/x", ok_handler, 0, 0), HttpErr::OK);
    CHECK_EQ_INT(http_transport_mock_rx(&mock, "POST /x HTTP/1.1\r\nContent-Length: 4\r\n\r\nab"), HttpErr::OK);
    CHECK_EQ_INT(http_transport_mock_tx_len(&mock), 0);
    CHECK_EQ_INT(http_transport_mock_rx(&mock, "cd"), HttpErr::OK);
    CHECK_CONTAINS(http_transport_mock_tx(&mock), "HTTP/1.1 200 OK");
    http_transport_mock_close(&mock);

    http_transport_mock_init(&mock);
    CHECK_EQ_INT(mock.engine.add_route(HttpMethod::GET, "/large", large_ram_handler, 0, 0), HttpErr::OK);
    CHECK_EQ_INT(http_transport_mock_rx(&mock, "GET /large HTTP/1.1\r\n\r\n"), HttpErr::OK);
    CHECK_CONTAINS(http_transport_mock_tx(&mock), "HTTP/1.1 200 OK");
    CHECK_CONTAINS(http_transport_mock_tx(&mock), "Content-Length: 1024");
    CHECK_TRUE(http_transport_mock_tx_len(&mock) > HTTP_RESPONSE_BUFFER_SIZE);
    http_transport_mock_close(&mock);

    http_transport_mock_init(&mock);
    CHECK_EQ_INT(mock.engine.add_route(HttpMethod::GET, "/stream", stream_handler, 0, 0), HttpErr::OK);
    CHECK_EQ_INT(http_transport_mock_rx(&mock, "GET /stream HTTP/1.1\r\n\r\n"), HttpErr::OK);
    CHECK_CONTAINS(http_transport_mock_tx(&mock), "HTTP/1.1 200 OK");
    CHECK_CONTAINS(http_transport_mock_tx(&mock), "Content-Length: 712");
    CHECK_TRUE(http_transport_mock_tx_len(&mock) > HTTP_RESPONSE_BUFFER_SIZE);
    http_transport_mock_close(&mock);

    return true;
}
