# esp8266-http-server-async

Lightweight, bounded HTTP server library for ESP8266 RTOS SDK, built on
`esp8266-tcp-transport`.

This library provides a small HTTP layer for embedded configuration pages,
local REST-style endpoints, and tiny static assets. It is not a general Web
framework.

French documentation: [README.fr.md](README.fr.md)

## V1 Scope

- ESP8266 RTOS SDK and PlatformIO.
- No Arduino dependency.
- No STL, exceptions, or RTTI in the library code.
- One TCP connection handles one HTTP request, emits one response, then closes.
- `Connection: close` is always emitted.
- Runtime buffers are bounded by [include/http_config.h](include/http_config.h).
- Static files are served through an application-provided `HttpFsBackend`.
- The public headers do not expose the TCP backend.

Out of scope for V1: keep-alive, WebSocket, SSE, native TLS, large uploads,
active dynamic route parameters, and template rendering.

## Install

Add the library to a PlatformIO project that targets `esp8266-rtos-sdk` and
provides `esp8266-tcp-transport`.

```bash
platformio pkg install
```

## Minimal Example

```cpp
#include "http_server.h"

static HttpServer server(80);

static void handle_root(HttpRequest &, HttpResponse &res)
{
    res.send_html("<!doctype html><html><body><h1>ok</h1></body></html>");
}

extern "C" void app_main(void)
{
    server.on(HttpMethod::GET, "/", handle_root);
    server.begin();
}
```

More examples are available in [examples](examples).

## Documentation

- [Documentation index](docs/README.md)
- [API contract](docs/api.md)
- [Function reference](docs/functions.md)
- [Architecture](docs/architecture.md)
- [Memory budget](docs/memory.md)
- [Static files](docs/static-files.md)
- [Tests](docs/tests.md)
- [Manual test matrix](docs/manual-test-matrix.md)

## Host Tests

```bash
pio run -c test/platformio.ini
PLATFORMIO_SETTING_ENABLE_TELEMETRY=no pio test -c test/platformio.ini
```

## Hardware Test

```bash
cp test/hardware_basic/platformio.local.ini.example test/hardware_basic/platformio.local.ini
pio run -d test/hardware_basic -e esp12e
pio run -d test/hardware_basic -e esp12e -t upload
pio device monitor -d test/hardware_basic -b 74880
```
