# esp8266-http-server-async

Bibliotheque serveur HTTP legere et bornee pour ESP8266 RTOS SDK, construite
au-dessus de `esp8266-tcp-transport`.

La bibliotheque fournit une couche HTTP minimale pour pages de configuration
embarquees, endpoints locaux de type REST, et petits fichiers statiques. Ce
n'est pas un framework Web generaliste.

Documentation anglaise prioritaire : [README.md](README.md)

## Perimetre V1

- ESP8266 RTOS SDK et PlatformIO.
- Aucune dependance Arduino.
- Pas de STL, exceptions ou RTTI dans le code de la bibliotheque.
- Une connexion TCP traite une requete HTTP, emet une reponse, puis ferme.
- `Connection: close` est toujours emis.
- Les buffers sont bornes par [include/http_config.h](include/http_config.h).
- Les fichiers statiques passent par un `HttpFsBackend` fourni par
  l'application.
- Les headers publics n'exposent pas le backend TCP.

Hors perimetre V1 : keep-alive, WebSocket, SSE, TLS natif, gros uploads,
parametres de route dynamiques actifs, et moteur de template.

## Installation

Ajouter la bibliotheque dans un projet PlatformIO ciblant `esp8266-rtos-sdk` et
fournissant `esp8266-tcp-transport`.

```bash
platformio pkg install
```

## Exemple minimal

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

Les autres exemples sont dans [examples](examples).

## Documentation

- [Index de documentation](docs/README.fr.md)
- [Contrat API](docs/api.fr.md)
- [Recueil des fonctions](docs/functions.fr.md)
- [Architecture](docs/architecture.fr.md)
- [Budget memoire](docs/memory.fr.md)
- [Fichiers statiques](docs/static-files.fr.md)
- [Tests](docs/tests.fr.md)
- [Matrice de test manuel](docs/manual-test-matrix.fr.md)

## Tests host

```bash
pio run -c test/platformio.ini
PLATFORMIO_SETTING_ENABLE_TELEMETRY=no pio test -c test/platformio.ini
```

## Test materiel

```bash
cp test/hardware_basic/platformio.local.ini.example test/hardware_basic/platformio.local.ini
pio run -d test/hardware_basic -e esp12e
pio run -d test/hardware_basic -e esp12e -t upload
pio device monitor -d test/hardware_basic -b 74880
```
