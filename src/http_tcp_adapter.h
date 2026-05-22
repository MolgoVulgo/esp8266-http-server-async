#ifndef HTTP_TCP_ADAPTER_H
#define HTTP_TCP_ADAPTER_H

#include <stdint.h>
#include "http_types.h"

class HttpEngine;

HttpErr http_tcp_adapter_start(uint16_t port, uint8_t max_clients, HttpEngine *engine);
void http_tcp_adapter_stop();

#endif
