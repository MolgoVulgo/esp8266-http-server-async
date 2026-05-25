# Architecture

French version: [architecture.fr.md](architecture.fr.md)

## Layers

```text
Application
  -> HttpServer / HttpRequest / HttpResponse
  -> HttpEngine
  -> HttpParser + HttpRouter + HttpStaticFiles
  -> http_tcp_adapter
  -> esp8266-tcp-transport
  -> ESP8266 RTOS SDK / lwIP
```

## Boundaries

- Public headers live in `include/`.
- The HTTP engine, parser, router, response builder, and static file resolver
  live in `src/`.
- TCP integration is isolated behind `src/http_tcp_adapter.*`.
- Public headers must not include or expose TCP transport types.
- Host tests drive `HttpEngine` through `test/test_host/http_transport_mock.*`.

## Request Flow

1. The TCP adapter accepts a client and asks `HttpEngine` for a slot.
2. Received bytes are fed to `HttpParser`.
3. When parsing completes, `HttpRouter` resolves application routes.
4. If no route matches and the method is `GET`, `HttpStaticFiles` may resolve a
   static file.
5. `HttpResponse` builds the full response in the slot TX buffer.
6. The adapter sends the response and closes the connection.

## V1 Connection Model

V1 uses a strict model:

```text
one TCP connection -> one HTTP request -> one HTTP response -> close
```

The model keeps RAM use predictable and avoids keep-alive state in the HTTP
engine.
