# Fichiers statiques

Version anglaise prioritaire : [static-files.md](static-files.md)

Le support des fichiers statiques est compile uniquement quand
`HTTP_ENABLE_STATIC_FILES == 1`.

## Contrat backend

`HttpFsBackend` est fourni par l'application et doit rester valide pendant son
utilisation par le serveur.

Callbacks obligatoires :

- `open`
- `read`
- `size`
- `close`

Callback optionnel :

- `exists`

`exists()` est seulement une optimisation. `open()` reste la source de verite.

## Regles de resolution

- Les routes enregistrees avec `on()` priment toujours sur les fichiers
  statiques.
- Les fichiers statiques sont consultes uniquement pour les requetes `GET`,
  apres un resultat `NOT_FOUND` du routeur.
- `serve_static("/static", "/www", &backend, "index.html")` mappe
  `/static/app.js` vers `/www/app.js`.
- Une requete sur le prefixe lui-meme mappe vers `index_file` s'il est fourni.

## Securite des chemins

Un chemin rejete ne doit jamais appeler le backend.

Motifs rejetes :

- chemin relatif vide ;
- `../` ;
- `..\` ;
- `//` ;
- `\\` ;
- valeur exacte `..` ;
- segment `'/..'`.

## Types MIME

Extensions connues :

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

Les extensions inconnues utilisent `application/octet-stream`.

## Limite de taille

Les fichiers statiques sont servis par chunks. La taille d'un fichier n'est
plus limitee par `HTTP_RESPONSE_BUFFER_SIZE`; cette macro borne seulement la
taille des chunks TX par client.
