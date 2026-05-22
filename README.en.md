# esp8266-http-server-async

Lightweight HTTP server library for ESP8266 RTOS SDK, built on `esp8266-tcp-transport`.

V1 is bounded and intentionally small: no Arduino, no STL/exceptions/RTTI in the library, one request per TCP connection, short responses, static file backend support, and `Connection: close` on every response.

See `docs/api.md`, `docs/memory.md`, `docs/static-files.md`, and `docs/tests.md` for the active contracts.
