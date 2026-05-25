# API Contract

French version: [api.fr.md](api.fr.md)

## Public Headers

- `http_config.h`
- `http_types.h`
- `http_request.h`
- `http_response.h`
- `http_fs_backend.h` when `HTTP_ENABLE_STATIC_FILES == 1`
- `http_server.h`

The public API is C++ only in V1. Public headers do not expose
`esp8266-tcp-transport` types.

## Server

`HttpServer` owns the HTTP entry point for an application. The constructor
takes a listen port and an optional maximum client count. The maximum client
count is clamped by `HTTP_MAX_CLIENTS`.

`HttpServer::on()` registers an application route. The route table is bounded by
`HTTP_MAX_ROUTES`. If `matcher == nullptr`, the exact matcher
`http_route_match_exact()` is used.

`HttpServer::serve_static()` registers a static file prefix when static files
are enabled. Static routes are bounded by `HTTP_MAX_STATIC_ROUTES`.

`HttpServer::begin()` starts the TCP adapter. `stop()` stops it and resets the
engine state.

## Routing

Resolution order is fixed:

1. Routes registered with `on()`.
2. Static file prefixes registered with `serve_static()`.
3. `404 Not Found`.

An `on()` route always takes priority over static files, regardless of
registration order.

If a path exists through `on()` with another method, the server returns
`405 Method Not Allowed` and does not check static files.

## Request Lifetime

`HttpRequest` is a temporary view over parser-owned buffers. Values returned by
`path()`, `query_raw()`, `header()`, and `body()` are valid only during the
handler call. Copy values into application storage if they must survive the
callback.

`param()` and `form_param()` decode URL-encoded values into caller-provided
buffers. `route_param()` is reserved for a later version and returns `false` in
V1.

## Response Contract

`HttpResponse::send()` and `HttpResponse::end()` may be called only once. A
second call returns `HttpErr::ALREADY_SENT`.

Every response includes:

- status line;
- optional `Content-Type`;
- `Content-Length`;
- `Connection: close`.

Handlers must call `send()`, `send_json()`, `send_html()`, or `end()`. If a
handler returns without sending a response, the engine emits
`500 Internal Server Error`.

The complete response, including headers and body, must fit in
`HTTP_RESPONSE_BUFFER_SIZE`.

## Parser and Errors

Supported methods are `GET`, `POST`, `PUT`, `DELETE`, and `OPTIONS`. Unknown
methods become `HttpMethod::UNKNOWN`.

Malformed requests return `400 Bad Request` and the engine reports
`HttpErr::PARSE_ERROR`. Bodies larger than `HTTP_BODY_MAX_SIZE` return
`413 Payload Too Large`.

Static file read inconsistencies return `500 Internal Server Error` and report
`HttpErr::FS_ERROR` when the error can be distinguished from response generation
failure.
