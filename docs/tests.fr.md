# Tests

Version anglaise prioritaire : [tests.md](tests.md)

## Tests host

Les tests host couvrent les limites de build, le decodage URL, le parser, les
requetes, les reponses, le routage, la securite des chemins statiques, la
resolution MIME et le moteur HTTP avec transport simule.

Fichiers de test :

- `test_build.cpp`
- `test_url_decode.cpp`
- `test_parser.cpp`
- `test_request.cpp`
- `test_response.cpp`
- `test_router.cpp`
- `test_static_path.cpp`
- `test_mime.cpp`
- `test_engine.cpp`

Commandes :

```bash
pio run -c test/platformio.ini
PLATFORMIO_SETTING_ENABLE_TELEMETRY=no pio test -c test/platformio.ini
```

## Test materiel

La validation materielle est dans `test/hardware_basic`.

```bash
cp test/hardware_basic/platformio.local.ini.example test/hardware_basic/platformio.local.ini
pio run -d test/hardware_basic -e esp12e
pio run -d test/hardware_basic -e esp12e -t upload
pio device monitor -d test/hardware_basic -b 74880
```

Remplacer `<ip>` par l'adresse affichee dans le moniteur serie.

```bash
curl -v http://<ip>/
curl -v http://<ip>/api/status
curl -v -X POST http://<ip>/api/config -d 'name=test'
curl -v http://<ip>/missing
curl -v -X PUT http://<ip>/
```

Voir [manual-test-matrix.fr.md](manual-test-matrix.fr.md) pour la checklist
complete.
