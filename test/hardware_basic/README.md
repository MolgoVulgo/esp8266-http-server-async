# ESP8266 HTTP Hardware Test

French version: [README.fr.md](README.fr.md)

This project validates the HTTP library on ESP8266 RTOS SDK with
`esp8266-tcp-transport`.

## Wi-Fi Configuration

```bash
cp test/hardware_basic/platformio.local.ini.example test/hardware_basic/platformio.local.ini
```

Set `WIFI_SSID` and `WIFI_PASSWORD`.

## Build, Upload, Monitor

```bash
pio run -d test/hardware_basic -e esp12e
pio run -d test/hardware_basic -e esp12e -t upload
pio device monitor -d test/hardware_basic -b 74880
```

## Curl Checks

Replace `<ip>` with the address printed in the serial monitor.

```bash
curl -v http://<ip>/
curl -v http://<ip>/api/status
curl -v -X POST http://<ip>/api/config -d 'name=test'
curl -v http://<ip>/missing
curl -v -X PUT http://<ip>/
```

Expected results:

- `GET /`: `200 OK`
- `GET /api/status`: JSON `200 OK`
- `POST /api/config` with `name`: `204 No Content`
- missing route: `404 Not Found`
- wrong method on `/`: `405 Method Not Allowed`
- every response contains `Connection: close`
