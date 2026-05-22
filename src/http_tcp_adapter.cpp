#include "http_tcp_adapter.h"

#include "http_engine.h"
#include "http_config.h"

#if HTTP_DEBUG
#include <stdio.h>
#define HTTP_DBG(fmt, ...) printf("http_tcp: " fmt "\n", ##__VA_ARGS__)
#else
#define HTTP_DBG(fmt, ...)
#endif

#ifndef HTTP_TCP_ADAPTER_STUB
#include "tcp_transport.h"
#endif

static HttpEngine *g_engine = 0;

#ifndef HTTP_TCP_ADAPTER_STUB
static void on_connect(tcp_conn_t *conn)
{
    if (g_engine == 0) {
        HTTP_DBG("connect without engine");
        tcp_close(conn);
        return;
    }

    int slot = g_engine->alloc_slot(conn);
    if (slot < 0) {
        HTTP_DBG("connect refused no slot");
        tcp_close(conn);
        return;
    }
    HTTP_DBG("connect slot=%d", slot);
}

static void on_data(tcp_conn_t *conn, const uint8_t *buf, size_t len)
{
    if (g_engine == 0) {
        HTTP_DBG("data without engine len=%u", (unsigned)len);
        tcp_close(conn);
        return;
    }
    int slot = g_engine->find_slot(conn);
    if (slot < 0) {
        HTTP_DBG("data without slot len=%u", (unsigned)len);
        tcp_close(conn);
        return;
    }
    HTTP_DBG("rx slot=%d len=%u", slot, (unsigned)len);
    g_engine->on_data(slot, buf, len);
    if (g_engine->tx_len(slot) > 0) {
        size_t sent = tcp_send(conn, g_engine->tx_data(slot), g_engine->tx_len(slot));
        HTTP_DBG("tx slot=%d len=%u sent=%u",
                 slot,
                 (unsigned)g_engine->tx_len(slot),
                 (unsigned)sent);
    }
    if (g_engine->close_after_send(slot)) {
        int close_ret = tcp_close_after_drain(conn);
        HTTP_DBG("close after drain slot=%d ret=%d", slot, close_ret);
    }
}

static void on_close(tcp_conn_t *conn)
{
    if (g_engine != 0) {
        int slot = g_engine->find_slot(conn);
        if (slot >= 0) {
            HTTP_DBG("close slot=%d", slot);
            g_engine->release_slot(slot);
        }
    }
}

static void on_error(tcp_conn_t *conn, int err)
{
    HTTP_DBG("error err=%d", err);
    (void)err;
    on_close(conn);
}
#endif

HttpErr http_tcp_adapter_start(uint16_t port, uint8_t max_clients, HttpEngine *engine)
{
    if (engine == 0 || port == 0) {
        return HttpErr::INVALID_ARG;
    }
    g_engine = engine;
#ifndef HTTP_TCP_ADAPTER_STUB
    tcp_server_callbacks_t callbacks;
    callbacks.on_connect = on_connect;
    callbacks.on_data = on_data;
    callbacks.on_close = on_close;
    callbacks.on_error = on_error;
    int r = tcp_server_start(port, max_clients, &callbacks);
    HTTP_DBG("start port=%u max_clients=%u ret=%d",
             (unsigned)port,
             (unsigned)max_clients,
             r);
    return r == TCP_TRANSPORT_OK ? HttpErr::OK : HttpErr::SEND_FAILED;
#else
    (void)max_clients;
    return HttpErr::OK;
#endif
}

void http_tcp_adapter_stop()
{
#ifndef HTTP_TCP_ADAPTER_STUB
    tcp_server_stop();
#endif
    g_engine = 0;
}
