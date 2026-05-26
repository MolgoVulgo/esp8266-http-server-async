# Budget memoire

Version anglaise prioritaire : [memory.md](memory.md)

Le modele memoire est statique et borne. La plupart des limites sont des macros
compile-time dans [include/http_config.h](../include/http_config.h).

## Limites par defaut

| Macro | Defaut | Impact |
| --- | ---: | --- |
| `HTTP_MAX_CLIENTS` | 3 | Nombre de slots clients HTTP. |
| `HTTP_MAX_ROUTES` | 16 | Routes applicatives enregistrees avec `on()`. |
| `HTTP_MAX_STATIC_ROUTES` | 4 | Prefixes de routes statiques. |
| `HTTP_LINE_MAX` | 128 | Limite de ligne de requete et de header. |
| `HTTP_HEADER_MAX_SIZE` | 512 | Texte total stocke des headers par client. |
| `HTTP_HEADER_MAX_COUNT` | 10 | Nombre de headers stockes par client. |
| `HTTP_HEADER_NAME_MAX` | 31 | Longueur d'un nom de header de requete. |
| `HTTP_BODY_MAX_SIZE` | 1024 | Taille maximale acceptee du body. |
| `HTTP_BODY_READ_CHUNK` | 128 | Taille de chunk reservee pour lecture body. |
| `HTTP_QUERY_MAX_SIZE` | 128 | Longueur de query string brute. |
| `HTTP_RESPONSE_BUFFER_SIZE` | 512 | Buffer de chunk TX HTTP utilise par le moteur. |
| `HTTP_FS_BLOCK_SIZE` | 512 | Macro de taille de bloc fichier statique. |
| `HTTP_RESP_HEADER_MAX` | 6 | Headers de reponse personnalises. |
| `HTTP_RESP_HEADER_NAME_MAX` | 31 | Longueur d'un nom de header de reponse. |
| `HTTP_RESP_HEADER_VALUE_MAX` | 96 | Longueur d'une valeur de header de reponse. |
| `HTTP_STATIC_PATH_MAX` | 160 | Longueur de chemin fichier construit. |

## Cout par client

Chaque slot client actif possede l'etat parser, le stockage de requete et un
buffer TX de reponse. Les buffers dominants par client sont :

- stockage headers de requete : `HTTP_HEADER_MAX_SIZE` ;
- stockage body de requete : `HTTP_BODY_MAX_SIZE` ;
- stockage TX de reponse : `HTTP_RESPONSE_BUFFER_SIZE` ;
- ligne parser, chemin, query et metadonnees fixes.

Augmenter `HTTP_MAX_CLIENTS`, `HTTP_BODY_MAX_SIZE` ou
`HTTP_RESPONSE_BUFFER_SIZE` augmente directement le cout RAM.

## Contrainte de reponse

Les reponses sont emises en streaming par `HttpEngine` (headers puis chunks de
body). `HTTP_RESPONSE_BUFFER_SIZE` est une taille de travail par client, pas la
taille maximale d'une reponse.

## Contrainte fichiers statiques

Les fichiers statiques sont lus et emis par morceaux. Un fichier n'a plus
besoin de tenir dans un buffer unique.
