# Architecture

```text
Application
  -> HttpServer / HttpRequest / HttpResponse
  -> HttpEngine
  -> HttpParser + HttpRouter + HttpStaticFiles
  -> http_tcp_adapter
  -> esp8266-tcp-transport
```

Les headers publics ne dépendent pas du transport TCP. L'adaptateur TCP est limité à `src/http_tcp_adapter.cpp`.

Le moteur HTTP est pilotable côté host via `test/test_host/http_transport_mock.*`.
