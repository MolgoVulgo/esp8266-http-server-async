# Tests

Tests host présents :

- `test_build.cpp`
- `test_url_decode.cpp`
- `test_parser.cpp`
- `test_request.cpp`
- `test_response.cpp`
- `test_router.cpp`
- `test_static_path.cpp`
- `test_mime.cpp`
- `test_engine.cpp`

Commande host validée :

```bash
pio run -c test/platformio.ini
PLATFORMIO_SETTING_ENABLE_TELEMETRY=no pio test -c test/platformio.ini

g++ -std=c++11 -Iinclude -Isrc test/test_host/*.cpp src/*.cpp -o /tmp/esp8266_http_host_tests
/tmp/esp8266_http_host_tests
```

Tests matériels à lancer sur ESP8266 :

```bash
cp test/hardware_basic/platformio.local.ini.example test/hardware_basic/platformio.local.ini
pio run -d test/hardware_basic -e esp12e
pio run -d test/hardware_basic -e esp12e -t upload
pio device monitor -d test/hardware_basic -b 74880

curl -v http://<ip>/
curl -v http://<ip>/api/status
curl -v -X POST http://<ip>/api/config -d 'name=test'
curl -v http://<ip>/not-found
curl -v -X PUT http://<ip>/
curl -v http://<ip>/static/index.html
curl -v http://<ip>/static/../secret.txt
```
