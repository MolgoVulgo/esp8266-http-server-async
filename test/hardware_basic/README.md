# Test matériel HTTP ESP8266

Ce projet valide la bibliothèque HTTP sur ESP8266 RTOS SDK avec `esp8266-tcp-transport`.

## Configuration Wi-Fi

```bash
cp test/hardware_basic/platformio.local.ini.example test/hardware_basic/platformio.local.ini
```

Adapter `WIFI_SSID` et `WIFI_PASSWORD`.

## Build / upload / logs

```bash
pio run -d test/hardware_basic -e esp12e
pio run -d test/hardware_basic -e esp12e -t upload
pio device monitor -d test/hardware_basic -b 74880
```

## Tests curl

Remplacer `<ip>` par l'adresse affichée dans le moniteur série.

```bash
curl -v http://<ip>/
curl -v http://<ip>/api/status
curl -v -X POST http://<ip>/api/config -d 'name=test'
curl -v http://<ip>/missing
curl -v -X PUT http://<ip>/
```

Résultats attendus :

- `GET /` : `200 OK`
- `GET /api/status` : JSON `200 OK`
- `POST /api/config` avec `name` : `204 No Content`
- route absente : `404 Not Found`
- mauvaise méthode sur `/` : `405 Method Not Allowed`
- chaque réponse contient `Connection: close`
