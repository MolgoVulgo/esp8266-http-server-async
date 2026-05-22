# API V1

Headers publics :

- `http_config.h`
- `http_types.h`
- `http_request.h`
- `http_response.h`
- `http_fs_backend.h`
- `http_server.h`

`HttpServer::on()` enregistre une route bornée par `HTTP_MAX_ROUTES`. Si `matcher == nullptr`, le matcher exact `http_route_match_exact` est utilisé.

`HttpRequest` expose des pointeurs vers des buffers internes. `path()`, `query_raw()` et `body()` sont valides uniquement pendant le callback handler.

`HttpResponse::send()` et `end()` ne peuvent être appelés qu'une fois. Un second envoi retourne `HttpErr::ALREADY_SENT`. `Connection: close` et `Content-Length` sont toujours générés.

`serve_static()` est disponible si `HTTP_ENABLE_STATIC_FILES == 1`. `HttpFsBackend::exists()` est optionnel ; `open()` reste la source de vérité.
