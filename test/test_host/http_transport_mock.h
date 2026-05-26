#ifndef HTTP_TRANSPORT_MOCK_H
#define HTTP_TRANSPORT_MOCK_H

#include <stddef.h>
#include <stdint.h>
#include "http_engine.h"

struct HttpTransportMock {
    HttpEngine engine;
    int slot;
    uint8_t out[4096];
    size_t out_len;
};

void http_transport_mock_init(HttpTransportMock *mock);
HttpErr http_transport_mock_rx(HttpTransportMock *mock, const char *data);
const char *http_transport_mock_tx(HttpTransportMock *mock);
size_t http_transport_mock_tx_len(HttpTransportMock *mock);
void http_transport_mock_close(HttpTransportMock *mock);

#endif
