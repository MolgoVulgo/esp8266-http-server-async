#include "test_host.h"

#include "http_transport_mock.h"

static void ok_handler(HttpRequest &, HttpResponse &res)
{
    res.send("ok");
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

    return true;
}
