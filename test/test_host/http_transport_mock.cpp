#include "http_transport_mock.h"

#include <string.h>

void http_transport_mock_init(HttpTransportMock *mock)
{
    mock->engine = HttpEngine();
    mock->slot = mock->engine.alloc_slot(mock);
    mock->out_len = 0;
    memset(mock->out, 0, sizeof(mock->out));
}

HttpErr http_transport_mock_rx(HttpTransportMock *mock, const char *data)
{
    HttpErr err = mock->engine.on_data(mock->slot,
                                       reinterpret_cast<const uint8_t *>(data),
                                       strlen(data));

    for (int i = 0; i < 64; i++) {
        if (mock->engine.tx_len(mock->slot) == 0) {
            HttpErr drain_err = mock->engine.on_drain(mock->slot);
            if (drain_err != HttpErr::OK) {
                break;
            }
            if (mock->engine.tx_len(mock->slot) == 0) {
                break;
            }
        }

        size_t pending = mock->engine.tx_len(mock->slot);
        if (mock->out_len + pending > sizeof(mock->out)) {
            return HttpErr::SEND_FAILED;
        }
        memcpy(mock->out + mock->out_len, mock->engine.tx_data(mock->slot), pending);
        mock->out_len += pending;
        mock->engine.on_bytes_sent(mock->slot, pending);
    }

    if (mock->out_len < sizeof(mock->out)) {
        mock->out[mock->out_len] = 0;
    }
    return err;
}

const char *http_transport_mock_tx(HttpTransportMock *mock)
{
    return reinterpret_cast<const char *>(mock->out);
}

size_t http_transport_mock_tx_len(HttpTransportMock *mock)
{
    return mock->out_len;
}

void http_transport_mock_close(HttpTransportMock *mock)
{
    mock->engine.release_slot(mock->slot);
    mock->slot = -1;
}
