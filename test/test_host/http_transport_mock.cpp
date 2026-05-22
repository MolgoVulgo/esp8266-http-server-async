#include "http_transport_mock.h"

#include <string.h>

void http_transport_mock_init(HttpTransportMock *mock)
{
    mock->engine = HttpEngine();
    mock->slot = mock->engine.alloc_slot(mock);
}

HttpErr http_transport_mock_rx(HttpTransportMock *mock, const char *data)
{
    return mock->engine.on_data(mock->slot,
                                reinterpret_cast<const uint8_t *>(data),
                                strlen(data));
}

const char *http_transport_mock_tx(HttpTransportMock *mock)
{
    return reinterpret_cast<const char *>(mock->engine.tx_data(mock->slot));
}

size_t http_transport_mock_tx_len(HttpTransportMock *mock)
{
    return mock->engine.tx_len(mock->slot);
}

void http_transport_mock_close(HttpTransportMock *mock)
{
    mock->engine.release_slot(mock->slot);
    mock->slot = -1;
}
