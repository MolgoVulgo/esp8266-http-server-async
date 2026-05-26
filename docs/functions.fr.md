# Recueil des fonctions

Version anglaise prioritaire : [functions.md](functions.md)

## Types

`HttpMethod`
: `GET`, `POST`, `PUT`, `DELETE`, `OPTIONS`, `UNKNOWN`.

`HttpErr`
: `OK`, `INVALID_ARG`, `NO_SLOT`, `ROUTE_FULL`, `HEADER_FULL`, `SEND_FAILED`,
  `PARSE_ERROR`, `TIMEOUT`, `FS_ERROR`, `ALREADY_SENT`, `NOT_STARTED`.

`HttpHandler`
: `void (*)(HttpRequest &req, HttpResponse &res)`.

`HttpRouteMatcher`
: `bool (*)(const char *pattern, const char *path, HttpRouteMatch *out_match)`.

## HttpServer

`HttpServer(uint16_t port, uint8_t max_clients = HTTP_MAX_CLIENTS)`
: Cree un serveur pour le port d'ecoute. `max_clients` est borne par
  `HTTP_MAX_CLIENTS`.

`HttpErr on(HttpMethod method, const char *path, HttpHandler handler, void *user_ctx = 0, HttpRouteMatcher matcher = 0)`
: Enregistre une route applicative. `path` et `handler` sont obligatoires.
  Retourne `INVALID_ARG` si l'entree est invalide et `ROUTE_FULL` si la table
  est pleine.

`HttpErr serve_static(const char *url_prefix, const char *fs_root, HttpFsBackend *backend, const char *index_file = "index.html")`
: Enregistre un prefixe de fichiers statiques quand
  `HTTP_ENABLE_STATIC_FILES == 1`. `backend->open`, `read`, `size` et `close`
  sont obligatoires. `exists` est optionnel.

`HttpErr begin()`
: Demarre l'adaptateur TCP HTTP. Un second appel apres demarrage retourne `OK`.

`void stop()`
: Arrete l'adaptateur s'il est demarre et reinitialise l'etat du moteur.

## HttpRequest

`HttpMethod method() const`
: Retourne la methode HTTP parse.

`const char *path() const`
: Retourne le chemin parse sans query string.

`const char *query_raw() const`
: Retourne la query string brute sans le `?`.

`const uint8_t *body() const`
: Retourne un pointeur vers le buffer du body.

`size_t body_len() const`
: Retourne la longueur du body.

`void *user_ctx() const`
: Retourne le pointeur de contexte associe a la route.

`const char *header(const char *name) const`
: Cherche un header par nom, sans tenir compte de la casse. Retourne `nullptr`
  si absent.

`bool param(const char *name, char *out, size_t out_len) const`
: Cherche et decode un parametre de query string dans `out`.

`bool form_param(const char *name, char *out, size_t out_len) const`
: Cherche et decode un parametre de formulaire si `Content-Type` commence par
  `application/x-www-form-urlencoded`.

`bool route_param(const char *name, char *out, size_t out_len) const`
: Reserve aux futurs parametres de route. Retourne toujours `false` en V1.

## HttpResponse

`void set_status(uint16_t status)`
: Definit le statut avant envoi de la reponse.

`HttpErr set_header(const char *name, const char *value)`
: Ajoute ou remplace un header avant envoi. Retourne `HEADER_FULL` si
  `HTTP_RESP_HEADER_MAX` est atteint.

`HttpErr set_content_type(const char *value)`
: Raccourci pour `Content-Type`.

`HttpErr send(const char *text)`
: Envoie une reponse `text/plain`.

`HttpErr send(const uint8_t *data, size_t len, const char *content_type = "application/octet-stream")`
: Envoie une reponse binaire avec le type donne.

`HttpErr send_json(const char *json)`
: Envoie une reponse `application/json`.

`HttpErr send_html(const char *html)`
: Envoie une reponse `text/html; charset=utf-8`.

`HttpErr send_progmem(const uint8_t *data_progmem, size_t len, const char *content_type)`
: Envoie une reponse dont le body provient de donnees flash/PROGMEM.

`HttpErr send_html_progmem(const char *html_progmem)`
: Raccourci pour du HTML stocke en flash/PROGMEM.

`HttpErr send_stream(HttpBodyReader reader, void *ctx, size_t content_length, const char *content_type)`
: Envoie un body stream par callback avec `Content-Length` fixe.

`HttpErr end(uint16_t status = 204)`
: Envoie une reponse vide avec le statut donne.

`bool sent() const`
: Indique si la reponse a deja ete generee.

`const uint8_t *data() const`, `size_t data_len() const`, `uint16_t status() const`
: Accesseurs de compatibilite. Le moteur construit et stream maintenant le TX.

## HttpFsBackend

`open(void *ctx, const char *path)`
: Ouvre un fichier et retourne un handle opaque, ou `nullptr` si absent.

`read(void *ctx, void *handle, uint8_t *buf, size_t len)`
: Lit jusqu'a `len` octets dans `buf`.

`size(void *ctx, void *handle)`
: Retourne la taille du fichier en octets.

`close(void *ctx, void *handle)`
: Ferme le handle.

`exists(void *ctx, const char *path)`
: Verification optionnelle d'existence. `open()` reste la source de verite.

## Helpers

`bool http_route_match_exact(const char *pattern, const char *path, HttpRouteMatch *out_match)`
: Matcher exact utilise par defaut lors de l'enregistrement d'une route.

`const char *http_method_name(HttpMethod method)`
: Retourne le nom texte d'une methode.

`HttpMethod http_method_from_cstr(const char *method)`
: Convertit un token HTTP en `HttpMethod`.

`const char *http_status_reason(uint16_t status)`
: Retourne la phrase de statut utilisee dans les reponses generees pour les
  statuts connus.
