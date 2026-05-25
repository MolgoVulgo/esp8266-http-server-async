# Test materiel HTTP ESP8266

Version anglaise prioritaire : [README.md](README.md)

Ce projet valide la bibliotheque HTTP sur ESP8266 RTOS SDK avec
`esp8266-tcp-transport`.

## Configuration Wi-Fi

```bash
cp test/hardware_basic/platformio.local.ini.example test/hardware_basic/platformio.local.ini
```

Adapter `WIFI_SSID` et `WIFI_PASSWORD`.

## Build, upload, logs

```bash
pio run -d test/hardware_basic -e esp12e
pio run -d test/hardware_basic -e esp12e -t upload
pio device monitor -d test/hardware_basic -b 74880
```

## Tests curl

Remplacer `<ip>` par l'adresse affichee dans le moniteur serie.

```bash
curl -v http://<ip>/
curl -v http://<ip>/api/status
curl -v -X POST http://<ip>/api/config -d 'name=test'
curl -v http://<ip>/missing
curl -v -X PUT http://<ip>/
```

Resultats attendus :

- `GET /` : `200 OK`
- `GET /api/status` : JSON `200 OK`
- `POST /api/config` avec `name` : `204 No Content`
- route absente : `404 Not Found`
- mauvaise methode sur `/` : `405 Method Not Allowed`
- chaque reponse contient `Connection: close`
