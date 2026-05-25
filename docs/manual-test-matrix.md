# Manual Test Matrix

French version: [manual-test-matrix.fr.md](manual-test-matrix.fr.md)

Use this checklist with the firmware in `test/hardware_basic` and a real ESP8266
target.

## Setup

- Build: `pio run -d test/hardware_basic -e esp12e`
- Upload: `pio run -d test/hardware_basic -e esp12e -t upload`
- Monitor: `pio device monitor -d test/hardware_basic -b 74880`
- Confirm the device prints an IP address.

## HTTP Checks

| Case | Command | Expected |
| --- | --- | --- |
| Root route | `curl -v http://<ip>/` | `200 OK` |
| JSON API | `curl -v http://<ip>/api/status` | `200 OK`, JSON body |
| Form POST | `curl -v -X POST http://<ip>/api/config -d 'name=test'` | `204 No Content` |
| Missing route | `curl -v http://<ip>/missing` | `404 Not Found` |
| Wrong method | `curl -v -X PUT http://<ip>/` | `405 Method Not Allowed` |
| Large body | POST body larger than `HTTP_BODY_MAX_SIZE` | `413 Payload Too Large` |
| Connection policy | Any request | `Connection: close` |

## Robustness Checks

- Client disconnects during request body upload.
- Client sends incomplete headers and disconnects.
- More than `HTTP_MAX_CLIENTS` clients connect at once.
- Static file backend returns a short read.
- Static traversal path such as `/static/../secret.txt` does not call the
  backend and does not expose a file.

Record firmware version, library commit, ESP8266 board, PlatformIO environment,
and any non-default HTTP macros with the result.
