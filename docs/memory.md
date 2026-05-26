# Memory Budget

French version: [memory.fr.md](memory.fr.md)

The memory model is static and bounded. Most limits are compile-time macros in
[include/http_config.h](../include/http_config.h).

## Default Limits

| Macro | Default | Impact |
| --- | ---: | --- |
| `HTTP_MAX_CLIENTS` | 3 | Number of HTTP client slots. |
| `HTTP_MAX_ROUTES` | 16 | Application routes registered with `on()`. |
| `HTTP_MAX_STATIC_ROUTES` | 4 | Static route prefixes. |
| `HTTP_LINE_MAX` | 128 | Request line and single header line limit. |
| `HTTP_HEADER_MAX_SIZE` | 512 | Total stored request header text per client. |
| `HTTP_HEADER_MAX_COUNT` | 10 | Stored request header entries per client. |
| `HTTP_HEADER_NAME_MAX` | 31 | Request header name length. |
| `HTTP_BODY_MAX_SIZE` | 1024 | Maximum accepted request body. |
| `HTTP_BODY_READ_CHUNK` | 128 | Reserved body read chunk size. |
| `HTTP_QUERY_MAX_SIZE` | 128 | Raw query string length. |
| `HTTP_RESPONSE_BUFFER_SIZE` | 512 | HTTP TX chunk buffer used by the engine. |
| `HTTP_FS_BLOCK_SIZE` | 512 | Static file block size macro. |
| `HTTP_RESP_HEADER_MAX` | 6 | Custom response headers. |
| `HTTP_RESP_HEADER_NAME_MAX` | 31 | Custom response header name length. |
| `HTTP_RESP_HEADER_VALUE_MAX` | 96 | Custom response header value length. |
| `HTTP_STATIC_PATH_MAX` | 160 | Built filesystem path length. |

## Per-Client Cost

Each active client slot owns parser state, request storage, and one TX response
buffer. The dominant per-client buffers are:

- request header storage: `HTTP_HEADER_MAX_SIZE`;
- request body storage: `HTTP_BODY_MAX_SIZE`;
- response TX storage: `HTTP_RESPONSE_BUFFER_SIZE`;
- parser line, path, query, and fixed metadata.

Increasing `HTTP_MAX_CLIENTS`, `HTTP_BODY_MAX_SIZE`, or
`HTTP_RESPONSE_BUFFER_SIZE` has a direct RAM cost.

## Response Constraint

Responses are streamed by `HttpEngine` (headers first, then body chunks).
`HTTP_RESPONSE_BUFFER_SIZE` is the per-client working chunk size, not the
maximum response size.

## Static File Constraint

Static files are read and sent in chunks. A file no longer needs to fit in one
buffer.
