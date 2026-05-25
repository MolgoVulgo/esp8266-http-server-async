# Contrat API

Version anglaise prioritaire : [api.md](api.md)

## Headers publics

- `http_config.h`
- `http_types.h`
- `http_request.h`
- `http_response.h`
- `http_fs_backend.h` quand `HTTP_ENABLE_STATIC_FILES == 1`
- `http_server.h`

L'API publique est uniquement C++ en V1. Les headers publics n'exposent aucun
type de `esp8266-tcp-transport`.

## Serveur

`HttpServer` est le point d'entree HTTP de l'application. Le constructeur prend
un port d'ecoute et un nombre maximal optionnel de clients. Cette valeur est
bornee par `HTTP_MAX_CLIENTS`.

`HttpServer::on()` enregistre une route applicative. La table est bornee par
`HTTP_MAX_ROUTES`. Si `matcher == nullptr`, le matcher exact
`http_route_match_exact()` est utilise.

`HttpServer::serve_static()` enregistre un prefixe de fichiers statiques quand
la fonction est activee. Les routes statiques sont bornees par
`HTTP_MAX_STATIC_ROUTES`.

`HttpServer::begin()` demarre l'adaptateur TCP. `stop()` l'arrete et reinitialise
l'etat du moteur.

## Routage

L'ordre de resolution est fixe :

1. Routes enregistrees avec `on()`.
2. Prefixes statiques enregistres avec `serve_static()`.
3. `404 Not Found`.

Une route `on()` prime toujours sur les fichiers statiques, quel que soit
l'ordre d'enregistrement.

Si un chemin existe via `on()` avec une autre methode, le serveur retourne
`405 Method Not Allowed` et ne consulte pas les fichiers statiques.

## Duree de vie de la requete

`HttpRequest` est une vue temporaire sur des buffers possedes par le parser. Les
valeurs retournees par `path()`, `query_raw()`, `header()` et `body()` sont
valides uniquement pendant l'appel du handler. Copier les valeurs dans un buffer
applicatif si elles doivent survivre au callback.

`param()` et `form_param()` decodent des valeurs URL-encodees dans des buffers
fournis par l'appelant. `route_param()` est reserve a une version ulterieure et
retourne `false` en V1.

## Contrat de reponse

`HttpResponse::send()` et `HttpResponse::end()` ne peuvent etre appeles qu'une
fois. Un second appel retourne `HttpErr::ALREADY_SENT`.

Chaque reponse contient :

- ligne de statut ;
- `Content-Type` optionnel ;
- `Content-Length` ;
- `Connection: close`.

Un handler doit appeler `send()`, `send_json()`, `send_html()` ou `end()`. Si le
handler retourne sans envoyer de reponse, le moteur emet
`500 Internal Server Error`.

La reponse complete, headers inclus, doit tenir dans
`HTTP_RESPONSE_BUFFER_SIZE`.

## Parser et erreurs

Les methodes supportees sont `GET`, `POST`, `PUT`, `DELETE` et `OPTIONS`. Les
methodes inconnues deviennent `HttpMethod::UNKNOWN`.

Une requete malformee retourne `400 Bad Request` et le moteur remonte
`HttpErr::PARSE_ERROR`. Un body plus grand que `HTTP_BODY_MAX_SIZE` retourne
`413 Payload Too Large`.

Une incoherence de lecture de fichier statique retourne
`500 Internal Server Error` et remonte `HttpErr::FS_ERROR` quand l'erreur peut
etre distinguee d'un echec de generation de reponse.
