# esp8266-http-server-async

Bibliothèque serveur HTTP légère pour ESP8266 RTOS SDK, au-dessus de `esp8266-tcp-transport`.

## Contraintes V1

- pas d'Arduino ;
- pas de STL, exceptions ou RTTI dans la bibliothèque ;
- une connexion, une requête, une réponse, puis `Connection: close` ;
- buffers bornés par `include/http_config.h` ;
- fichiers statiques servis via backend applicatif, sans exposer le TCP dans l'API publique.

## Exemple minimal

```cpp
#include "http_server.h"

static void handle_root(HttpRequest &, HttpResponse &res)
{
    res.send_html("<h1>ok</h1>");
}

extern "C" void app_main(void)
{
    HttpServer server(80);
    server.on(HttpMethod::GET, "/", handle_root);
    server.begin();
}
```

## Tests host

```bash
pio run -c test/platformio.ini
PLATFORMIO_SETTING_ENABLE_TELEMETRY=no pio test -c test/platformio.ini

g++ -std=c++11 -Iinclude -Isrc \
  test/test_host/test_runner.cpp test/test_host/test_build.cpp \
  test/test_host/test_url_decode.cpp test/test_host/test_parser.cpp \
  test/test_host/test_request.cpp test/test_host/test_response.cpp \
  test/test_host/test_router.cpp test/test_host/test_static_path.cpp \
  test/test_host/test_mime.cpp test/test_host/http_transport_mock.cpp \
  test/test_host/test_engine.cpp src/*.cpp \
  -o /tmp/esp8266_http_host_tests
/tmp/esp8266_http_host_tests
```

## Limites V1

Pas de keep-alive, WebSocket, SSE, TLS natif, upload de gros fichiers, routes dynamiques actives ni moteur de template.
