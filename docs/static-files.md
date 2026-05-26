# Static Files

French version: [static-files.fr.md](static-files.fr.md)

Static file support is compiled only when `HTTP_ENABLE_STATIC_FILES == 1`.

## Backend Contract

`HttpFsBackend` is provided by the application and must remain valid while the
server uses it.

Required callbacks:

- `open`
- `read`
- `size`
- `close`

Optional callback:

- `exists`

`exists()` is an optimization only. `open()` remains the source of truth.

## Resolution Rules

- Routes registered with `on()` always take priority over static files.
- Static files are checked only for `GET` requests after route lookup returns
  `NOT_FOUND`.
- `serve_static("/static", "/www", &backend, "index.html")` maps
  `/static/app.js` to `/www/app.js`.
- A request for the prefix itself maps to `index_file` when provided.

## Path Safety

Rejected paths never call the backend.

Rejected patterns include:

- empty relative path;
- `../`;
- `..\`;
- `//`;
- `\\`;
- exact `..`;
- any `'/..'` segment.

## MIME Types

Known extensions:

| Extension | MIME |
| --- | --- |
| `.html` | `text/html; charset=utf-8` |
| `.css` | `text/css` |
| `.js` | `application/javascript` |
| `.json` | `application/json` |
| `.ico` | `image/x-icon` |
| `.png` | `image/png` |
| `.svg` | `image/svg+xml` |
| `.txt` | `text/plain` |
| `.xml` | `application/xml` |

Unknown extensions use `application/octet-stream`.

## Size Limit

Static files are served in chunks. File size is no longer limited by
`HTTP_RESPONSE_BUFFER_SIZE`; this macro only bounds per-client TX chunk size.
