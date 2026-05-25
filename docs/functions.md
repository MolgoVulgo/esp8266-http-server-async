# Function Reference

French version: [functions.fr.md](functions.fr.md)

## Types

`HttpMethod`
: `GET`, `POST`, `PUT`, `DELETE`, `OPTIONS`, `UNKNOWN`.

`HttpErr`
: `OK`, `INVALID_ARG`, `NO_SLOT`, `ROUTE_FULL`, `HEADER_FULL`, `SEND_FAILED`,
  `PARSE_ERROR`, `TIMEOUT`, `FS_ERROR`, `ALREADY_SENT`, `NOT_STARTED`.

`HttpHandler`
: `void (*)(HttpRequest &req, HttpResponse &res)`.

`HttpRouteMatcher`
: `bool (*)(const char *pattern, const char *path, HttpRouteMatch *out_match)`.

## HttpServer

`HttpServer(uint16_t port, uint8_t max_clients = HTTP_MAX_CLIENTS)`
: Creates a server object for the listen port. `max_clients` is capped by
  `HTTP_MAX_CLIENTS`.

`HttpErr on(HttpMethod method, const char *path, HttpHandler handler, void *user_ctx = 0, HttpRouteMatcher matcher = 0)`
: Registers an application route. `path` and `handler` are required. Returns
  `INVALID_ARG` for invalid input and `ROUTE_FULL` when the route table is full.

`HttpErr serve_static(const char *url_prefix, const char *fs_root, HttpFsBackend *backend, const char *index_file = "index.html")`
: Registers a static file prefix when `HTTP_ENABLE_STATIC_FILES == 1`.
  `backend->open`, `read`, `size`, and `close` are required. `exists` is
  optional.

`HttpErr begin()`
: Starts the HTTP TCP adapter. Calling it again after a successful start returns
  `OK`.

`void stop()`
: Stops the adapter if started and resets the engine state.

## HttpRequest

`HttpMethod method() const`
: Returns the parsed HTTP method.

`const char *path() const`
: Returns the parsed path without the query string.

`const char *query_raw() const`
: Returns the raw query string without the leading `?`.

`const uint8_t *body() const`
: Returns a pointer to the request body buffer.

`size_t body_len() const`
: Returns the request body length.

`void *user_ctx() const`
: Returns the context pointer registered with the matched route.

`const char *header(const char *name) const`
: Looks up a header by case-insensitive name. Returns `nullptr` when absent.

`bool param(const char *name, char *out, size_t out_len) const`
: Finds and URL-decodes a query parameter into `out`.

`bool form_param(const char *name, char *out, size_t out_len) const`
: Finds and URL-decodes a form parameter when `Content-Type` starts with
  `application/x-www-form-urlencoded`.

`bool route_param(const char *name, char *out, size_t out_len) const`
: Reserved for future route parameters. Always returns `false` in V1.

## HttpResponse

`void set_status(uint16_t status)`
: Sets the response status before the response is sent.

`HttpErr set_header(const char *name, const char *value)`
: Adds or replaces a response header before send. Returns `HEADER_FULL` when
  `HTTP_RESP_HEADER_MAX` is reached.

`HttpErr set_content_type(const char *value)`
: Convenience wrapper for `Content-Type`.

`HttpErr send(const char *text)`
: Sends a `text/plain` response.

`HttpErr send(const uint8_t *data, size_t len, const char *content_type = "application/octet-stream")`
: Sends a binary response with the given content type.

`HttpErr send_json(const char *json)`
: Sends an `application/json` response.

`HttpErr send_html(const char *html)`
: Sends a `text/html; charset=utf-8` response.

`HttpErr end(uint16_t status = 204)`
: Sends an empty response with the given status.

`bool sent() const`
: Returns whether the response has already been generated.

`const uint8_t *data() const`, `size_t data_len() const`, `uint16_t status() const`
: Expose the generated response buffer, length, and status for the engine and
  host tests.

## HttpFsBackend

`open(void *ctx, const char *path)`
: Opens a file and returns an opaque handle, or `nullptr` when absent.

`read(void *ctx, void *handle, uint8_t *buf, size_t len)`
: Reads up to `len` bytes into `buf`.

`size(void *ctx, void *handle)`
: Returns the file size in bytes.

`close(void *ctx, void *handle)`
: Closes the file handle.

`exists(void *ctx, const char *path)`
: Optional existence check. `open()` remains the source of truth.

## Helpers

`bool http_route_match_exact(const char *pattern, const char *path, HttpRouteMatch *out_match)`
: Exact string matcher used by default route registration.

`const char *http_method_name(HttpMethod method)`
: Returns a method name string.

`HttpMethod http_method_from_cstr(const char *method)`
: Converts an HTTP method token to `HttpMethod`.

`const char *http_status_reason(uint16_t status)`
: Returns the reason phrase used in generated responses for known statuses.
