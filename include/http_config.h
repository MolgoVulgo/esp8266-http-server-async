#ifndef HTTP_CONFIG_H
#define HTTP_CONFIG_H

#ifndef HTTP_MAX_ROUTES
#define HTTP_MAX_ROUTES 16
#endif

#ifndef HTTP_MAX_CLIENTS
#define HTTP_MAX_CLIENTS 3
#endif

#ifndef HTTP_LINE_MAX
#define HTTP_LINE_MAX 128
#endif

#ifndef HTTP_HEADER_MAX_SIZE
#define HTTP_HEADER_MAX_SIZE 512
#endif

#ifndef HTTP_HEADER_MAX_COUNT
#define HTTP_HEADER_MAX_COUNT 10
#endif

#ifndef HTTP_HEADER_NAME_MAX
#define HTTP_HEADER_NAME_MAX 31
#endif

#ifndef HTTP_BODY_MAX_SIZE
#define HTTP_BODY_MAX_SIZE 1024
#endif

#ifndef HTTP_BODY_READ_CHUNK
#define HTTP_BODY_READ_CHUNK 128
#endif

#ifndef HTTP_QUERY_MAX_SIZE
#define HTTP_QUERY_MAX_SIZE 128
#endif

#ifndef HTTP_FS_BLOCK_SIZE
#define HTTP_FS_BLOCK_SIZE 512
#endif

#ifndef HTTP_TIMEOUT_HEADER_MS
#define HTTP_TIMEOUT_HEADER_MS 3000
#endif

#ifndef HTTP_TIMEOUT_BODY_MS
#define HTTP_TIMEOUT_BODY_MS 5000
#endif

#ifndef HTTP_TIMEOUT_IDLE_MS
#define HTTP_TIMEOUT_IDLE_MS 5000
#endif

#ifndef HTTP_RESP_HEADER_MAX
#define HTTP_RESP_HEADER_MAX 6
#endif

#ifndef HTTP_RESP_HEADER_NAME_MAX
#define HTTP_RESP_HEADER_NAME_MAX 31
#endif

#ifndef HTTP_RESP_HEADER_VALUE_MAX
#define HTTP_RESP_HEADER_VALUE_MAX 96
#endif

/* Maximum complete HTTP response size, including headers and body.
 * Must stay <= TCP_TX_BUFFER_SIZE from esp8266-tcp-transport in V1 because
 * the HTTP adapter sends one complete response into the transport TX buffer.
 * Static file body capacity is roughly HTTP_RESPONSE_BUFFER_SIZE - 120 bytes.
 */
#ifndef HTTP_RESPONSE_BUFFER_SIZE
#define HTTP_RESPONSE_BUFFER_SIZE 512
#endif

#ifndef HTTP_ENABLE_STATIC_FILES
#define HTTP_ENABLE_STATIC_FILES 1
#endif

#ifndef HTTP_ENABLE_JSON_HELPERS
#define HTTP_ENABLE_JSON_HELPERS 1
#endif

#ifndef HTTP_ENABLE_ROUTE_PARAMS
#define HTTP_ENABLE_ROUTE_PARAMS 0
#endif

#ifndef HTTP_DEBUG
#define HTTP_DEBUG 0
#endif

#ifndef HTTP_ROUTE_PARAM_MAX
#define HTTP_ROUTE_PARAM_MAX 4
#endif

#ifndef HTTP_PARAM_NAME_MAX
#define HTTP_PARAM_NAME_MAX 24
#endif

#ifndef HTTP_PARAM_VALUE_MAX
#define HTTP_PARAM_VALUE_MAX 48
#endif

#ifndef HTTP_MAX_STATIC_ROUTES
#define HTTP_MAX_STATIC_ROUTES 4
#endif

#ifndef HTTP_STATIC_PATH_MAX
#define HTTP_STATIC_PATH_MAX 160
#endif

#endif
