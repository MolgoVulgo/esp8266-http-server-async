# Cahier fonctionnel — Bibliothèque serveur Web ESP8266 RTOS SDK

## 1. Objet du document

Ce cahier fonctionnel définit le comportement attendu d’une bibliothèque serveur Web légère pour ESP8266, développée en C++ léger sur `esp8266-rtos-sdk`, sans Arduino, et construite au-dessus de la bibliothèque `esp8266-tcp-transport`.

Le document formalise le périmètre fonctionnel V1, les contraintes d’exécution, les contrats d’API publique, les règles de configuration, les comportements d’erreur, les besoins de documentation et les critères d’acceptation.

L’objectif n’est pas de reproduire `ESPAsyncWebServer`, mais de fournir une couche HTTP embarquée, bornée, prédictible et adaptée aux limites mémoire de l’ESP8266.

## 2. Positionnement de la bibliothèque

La bibliothèque serveur Web est une couche HTTP applicative.

Elle ne prend pas en charge directement la couche TCP bas niveau. Cette responsabilité appartient à `esp8266-tcp-transport`.

La séparation fonctionnelle est la suivante :

```text
Application utilisateur
  ↓
Bibliothèque serveur Web HTTP
  ↓
Adaptateur TCP HTTP
  ↓
esp8266-tcp-transport
  ↓
ESP8266 RTOS SDK / lwIP
```

La bibliothèque Web prend en charge :

- l’interprétation des requêtes HTTP ;
- le parsing de la ligne de requête, des headers, de la query string et du body ;
- le routage applicatif ;
- la construction des réponses HTTP ;
- le service de fichiers statiques borné ;
- la gestion des timeouts HTTP ;
- la traduction entre événements TCP et états HTTP ;
- l’exposition d’une API publique stable côté application.

La bibliothèque ne prend pas en charge :

- l’implémentation d’un serveur TCP brut ;
- la gestion Wi-Fi ;
- TLS natif ;
- WebSocket ;
- OTA ;
- DNS captif ;
- gestion utilisateur ou session ;
- moteur de template ;
- dépendance Arduino.

## 3. Objectifs fonctionnels V1

La V1 doit permettre d’embarquer un serveur HTTP local utilisable pour :

- une interface Web de configuration ;
- des pages HTML simples ;
- des API REST locales ;
- des commandes applicatives ;
- une supervision minimale ;
- l’intégration avec d’autres bibliothèques ESP8266 RTOS.

La bibliothèque doit rester volontairement limitée. Sa valeur fonctionnelle repose sur la maîtrise mémoire, l’exécution non bloquante, la robustesse sur clients imparfaits et la stabilité du contrat public.

## 4. Contraintes structurantes

### 4.1 Plateforme cible

La bibliothèque cible :

- ESP8266 ;
- ESP8266 RTOS SDK ;
- PlatformIO ;
- compilation C++ ;
- absence totale de dépendance Arduino ;
- absence de dépendance à `ESP8266WebServer` ;
- absence de dépendance à `ESPAsyncWebServer`.

### 4.2 Langage et style d’implémentation

La bibliothèque est écrite en C++ léger.

L’API publique est exposée en C++ uniquement pour la V1. La compatibilité C n’est pas garantie.

Règles V1 :

- API publique basée sur `class`, `struct`, `enum class`, références et callbacks C-style ;
- cœur interne impératif, sans abstraction coûteuse ;
- aucune STL ;
- aucune exception ;
- aucun RTTI ;
- pas de `std::string`, `std::vector`, `new`, `delete` non maîtrisés ;
- templates autorisés uniquement s’ils restent inline, simples et sans coût de génération incontrôlé.

L’objectif est de conserver une API lisible sans importer le coût mémoire et ABI d’un C++ complet.

### 4.3 Mémoire

La RAM est la contrainte principale.

La bibliothèque doit :

- borner le nombre de clients simultanés ;
- borner la taille des headers ;
- borner la taille du body ;
- borner la taille de la query string ;
- éviter le chargement complet de fichiers en RAM ;
- éviter les allocations dynamiques non maîtrisées ;
- utiliser des buffers statiques ou explicitement contrôlés ;
- rendre les tailles critiques configurables à la compilation ;
- documenter le coût RAM par client actif.

Le modèle cible est : peu de clients, petits buffers, machine d’états explicite, streaming des fichiers et fermeture systématique après réponse.

### 4.4 Exécution RTOS

Le serveur doit fonctionner sans bloquer la task réseau.

Règles d’exécution :

- aucun traitement long dans les callbacks TCP ;
- aucune attente active ;
- aucune boucle bloquante sur réception ou émission ;
- traitement progressif des requêtes ;
- fermeture propre des connexions invalides ;
- tolérance aux clients lents, incomplets ou silencieux ;
- état client isolé par slot ;
- absence d’impact d’une requête invalide sur les autres clients.

## 5. Périmètre fonctionnel V1

### 5.1 Démarrage et arrêt du serveur

La bibliothèque doit permettre à l’application de :

- créer une instance de serveur HTTP ;
- définir un port d’écoute ;
- définir un nombre maximal de clients ;
- enregistrer des routes ;
- enregistrer une racine de fichiers statiques si activée ;
- démarrer le serveur ;
- arrêter le serveur ;
- libérer ou réinitialiser l’état interne.

Le démarrage HTTP encapsule le démarrage TCP. L’utilisateur de la bibliothèque ne manipule pas directement `esp8266-tcp-transport` pour un usage HTTP standard.

Contrat attendu :

```cpp
HttpServer server(80);
server.on(HttpMethod::GET, "/", handle_root);
server.begin();
```

### 5.2 Méthodes HTTP supportées

La bibliothèque doit reconnaître les méthodes suivantes :

- `GET` ;
- `POST` ;
- `PUT` ;
- `DELETE` ;
- `OPTIONS`.

Toute méthode inconnue ou non supportée doit produire une réponse explicite, typiquement `405 Method Not Allowed`, si la requête est suffisamment parsée pour permettre une réponse HTTP.

### 5.3 Parsing HTTP minimal

La bibliothèque doit parser :

- la méthode ;
- le chemin sans query string ;
- la query string brute ;
- la version HTTP ;
- les headers ;
- le body si `Content-Length` est présent et compatible avec la limite configurée.

Le parsing doit être borné par :

- `HTTP_LINE_MAX` pour la ligne de requête ;
- `HTTP_HEADER_MAX_SIZE` pour la taille globale des headers ;
- `HTTP_HEADER_MAX_COUNT` pour le nombre de headers ;
- `HTTP_BODY_MAX_SIZE` pour le body ;
- `HTTP_QUERY_MAX_SIZE` pour la query string.

En cas de dépassement, la requête doit être rejetée proprement.

### 5.4 Modèle de connexion

En V1, le modèle est strict :

```text
une connexion TCP → une requête HTTP → une réponse HTTP → fermeture
```

La bibliothèque doit toujours envoyer :

```http
Connection: close
```

Le keep-alive est hors périmètre V1.

Après génération complète de la réponse, la connexion est marquée pour fermeture. La fermeture effective intervient uniquement après vidage du buffer TX par la couche transport, sauf erreur réseau.

### 5.5 Routage applicatif

La bibliothèque doit permettre l’association d’une méthode et d’un chemin à un handler applicatif.

Fonctions attendues :

- enregistrer une route exacte ;
- distinguer deux routes ayant le même chemin mais des méthodes différentes ;
- retourner `404 Not Found` si aucun chemin ne correspond ;
- retourner `405 Method Not Allowed` si le chemin existe mais pas pour la méthode demandée ;
- permettre un matcher personnalisé sans casser l’API V1 ;
- préparer l’évolution vers les routes dynamiques sans les implémenter en V1.

Règle obligatoire pour `405 Method Not Allowed` :

- le routeur recherche d’abord une correspondance complète méthode + chemin parmi les routes `on()` ;
- si aucune correspondance complète n’est trouvée, il doit parcourir toute la table des routes `on()` pour détecter une correspondance de chemin sans tenir compte de la méthode ;
- ce parcours est obligatoire, pas best-effort ;
- si un chemin correspond avec une autre méthode, le serveur retourne `405 Method Not Allowed` ;
- dans ce cas, les routes statiques ne sont pas consultées ;
- le coût reste borné par `HTTP_MAX_ROUTES`, fixé à 16 par défaut.

Ordre de résolution obligatoire :

1. routes applicatives enregistrées avec `on()` ;
2. fichiers statiques enregistrés avec `serve_static()` ;
3. fallback `404 Not Found`.

Une route `on()` prime toujours sur un fichier statique, quel que soit l’ordre d’enregistrement.

### 5.6 Requête applicative

Le handler reçoit une référence vers un objet `HttpRequest` valide uniquement pendant l’exécution du callback.

L’objet doit exposer :

- la méthode ;
- le chemin ;
- la query string brute ;
- le body brut ;
- la longueur du body ;
- le contexte utilisateur ;
- l’accès aux headers ;
- l’accès aux paramètres de query string ;
- l’accès aux paramètres de formulaire `application/x-www-form-urlencoded` ;
- l’accès futur aux paramètres de route.

Règle critique : les pointeurs `path`, `query_raw` et `body` pointent dans des buffers internes. Ils deviennent invalides dès la fin du handler.

L’application ne doit jamais conserver ces pointeurs.

Si une valeur doit survivre au callback, elle doit être copiée explicitement dans un buffer applicatif.

### 5.7 Réponse applicative

Le handler reçoit une référence vers `HttpResponse`.

L’objet doit permettre :

- de définir un code HTTP ;
- de définir des headers de réponse ;
- de définir le `Content-Type` ;
- d’envoyer une chaîne texte ;
- d’envoyer un buffer binaire court ;
- d’envoyer du JSON ;
- d’envoyer du HTML ;
- de terminer sans corps via `end()`.

La réponse ne peut être envoyée qu’une seule fois.

Comportements obligatoires :

| Cas | Comportement attendu |
|---|---|
| `send()` appelé une fois | Réponse générée et planifiée dans le buffer TX |
| `send()` appelé deux fois | Retourne `HttpErr::ALREADY_SENT` |
| `set_header()` après `send()` | Retourne `HttpErr::ALREADY_SENT` |
| `set_status()` après `send()` | Ignoré silencieusement |
| Limite headers réponse atteinte | Retourne `HttpErr::HEADER_FULL` |
| `send(const char*)` | Calcule automatiquement `Content-Length` |
| `send(data, len)` | Utilise la taille explicite |
| Absence de `Content-Type` | Défaut `text/plain` pour `send(const char*)` |
| Réponse terminée | Connexion marquée pour fermeture après vidage TX |
| Handler terminé sans `send()` ni `end()` | Bug applicatif traité comme `500 Internal Server Error` automatique si aucune réponse n’a encore été émise |

Si un handler applicatif retourne sans avoir appelé `send()` ni `end()`, la bibliothèque doit générer automatiquement une réponse courte `500 Internal Server Error`, marquer la connexion pour fermeture après vidage TX, puis libérer le slot. Ce comportement est obligatoire afin d’éviter une connexion pendante ou une fermeture brute silencieuse.

### 5.8 Codes HTTP minimaux

La bibliothèque doit gérer au minimum :

- `200 OK` ;
- `204 No Content` ;
- `400 Bad Request` ;
- `404 Not Found` ;
- `405 Method Not Allowed` ;
- `413 Payload Too Large` ;
- `500 Internal Server Error`.

Les réponses d’erreur doivent rester courtes, déterministes et sans génération HTML lourde.

### 5.9 Query string

La bibliothèque doit permettre :

- de lire la query string brute via `req.query_raw` ;
- de rechercher un paramètre nommé via `req.param()` ;
- de copier la valeur dans un buffer fourni par l’application ;
- de retourner `false` si le paramètre est absent ou si le buffer est insuffisant.

Le décodage V1 est minimal :

- `%XX` pour les caractères courants ;
- `+` converti en espace ;
- pas de support Unicode complet ;
- pas d’allocation dynamique.

### 5.10 Body de requête

La bibliothèque doit accepter un body borné pour :

- formulaire simple ;
- JSON court ;
- commande locale ;
- payload de configuration.

Règles :

- body limité par `HTTP_BODY_MAX_SIZE` ;
- réception possible par chunks internes ;
- rejet avec `413 Payload Too Large` si dépassement annoncé ou constaté ;
- timeout de connexion délégué au transport via `TCP_IDLE_TIMEOUT_MS` ;
- pas d’upload de gros fichiers en V1.

### 5.11 Formulaires URL-encodés

Pour les requêtes dont le `Content-Type` est `application/x-www-form-urlencoded`, la bibliothèque doit permettre l’extraction d’un paramètre via `req.form_param()`.

Règles :

- lecture uniquement depuis le body ;
- décodage identique à `param()` ;
- copie dans un buffer applicatif ;
- retour `false` si Content-Type incompatible, body absent, paramètre absent ou buffer insuffisant ;
- aucune allocation interne persistante.

### 5.12 Headers entrants

La bibliothèque doit gérer au minimum :

- `Host` ;
- `Content-Length` ;
- `Content-Type` ;
- `Connection` ;
- `User-Agent` en lecture facultative.

Tous les headers reçus consomment un slot dans `HTTP_HEADER_MAX_COUNT`, qu’ils soient connus ou non.

Si le nombre de headers dépasse la limite, la requête est rejetée avec `400 Bad Request` et la connexion est fermée proprement.

Ce comportement évite qu’un header critique placé après la limite soit ignoré silencieusement.

### 5.13 Fichiers statiques

Si `HTTP_ENABLE_STATIC_FILES` vaut `1`, la bibliothèque doit pouvoir servir des fichiers depuis un backend fourni par l’application.

L’application fournit une instance de `HttpFsBackend` contenant des callbacks :

- `open()` ;
- `read()` ;
- `size()` ;
- `close()` ;
- `exists()` facultatif.

Contrat de `exists()` :

- si `exists()` est fourni, il peut être utilisé comme optimisation préalable ;
- si `exists()` vaut `nullptr`, la bibliothèque utilise `open()` pour tester l’existence effective du fichier ;
- `open()` reste la source de vérité ;
- si `exists()` retourne `true` mais que `open()` retourne `nullptr`, la bibliothèque traite le fichier comme absent et retourne `404 Not Found`.

La bibliothèque reste responsable :

- du mapping URL vers chemin fichier ;
- de la sanitisation du chemin ;
- du choix MIME minimal ;
- de l’ouverture du fichier ;
- du streaming par blocs ;
- de la fermeture du handle ;
- du retour `404` en cas d’absence ou d’erreur d’ouverture.

Un handler applicatif ne lit jamais directement les fichiers statiques à travers `HttpFsBackend`.

### 5.14 Streaming fichier

Les fichiers statiques ne doivent jamais être chargés entièrement en RAM.

Le serveur doit envoyer les fichiers par blocs de `HTTP_FS_BLOCK_SIZE`.

Un état interne par client actif doit suivre :

- le handle fichier ;
- la taille totale ;
- le nombre d’octets déjà envoyés ;
- le type MIME ;
- l’état actif/inactif du streaming.

Le streaming se poursuit tant que le buffer TX du transport accepte de nouvelles données.

En fin de fichier ou en erreur, le handle doit être fermé.

### 5.15 Sanitisation des chemins statiques

Avant tout accès au backend fichier, la bibliothèque doit rejeter :

- tout chemin contenant `../` ;
- tout chemin contenant `..\` ;
- tout chemin contenant `//` ;
- tout chemin vide ;
- tout chemin `/` sans index configuré ;
- tout chemin dont la résolution sort de la racine statique.

Un chemin rejeté produit `404 Not Found` sans appel au backend fichier.

### 5.16 Types MIME minimaux

La bibliothèque doit reconnaître au minimum :

| Extension | Content-Type |
|---|---|
| `.html` | `text/html; charset=utf-8` |
| `.css` | `text/css` |
| `.js` | `application/javascript` |
| `.json` | `application/json` |
| `.ico` | `image/x-icon` |
| `.png` | `image/png` |
| `.svg` | `image/svg+xml` |
| `.txt` | `text/plain` |
| `.xml` | `application/xml` |

Toute extension inconnue doit produire :

```http
Content-Type: application/octet-stream
```

## 6. Contrat d’API publique

### 6.1 Types publics requis

L’API publique V1 doit exposer :

- `HttpServer` ;
- `HttpRequest` ;
- `HttpResponse` ;
- `HttpHandler` ;
- `HttpRouteMatcher` ;
- `HttpRouteMatch` ;
- `HttpFsBackend` si fichiers statiques activés ;
- `HttpMethod` ;
- `HttpErr`.

### 6.2 Méthodes HTTP

```cpp
enum class HttpMethod : uint8_t {
    GET     = 0,
    POST    = 1,
    PUT     = 2,
    DELETE  = 3,
    OPTIONS = 4,
    UNKNOWN = 0xFF
};
```

### 6.3 Codes d’erreur internes

```cpp
enum class HttpErr : int8_t {
    OK              =  0,
    INVALID_ARG     = -1,
    NO_SLOT         = -2,
    ROUTE_FULL      = -3,
    HEADER_FULL     = -4,
    SEND_FAILED     = -5,
    PARSE_ERROR     = -6,
    TIMEOUT         = -7,
    FS_ERROR        = -8,
    ALREADY_SENT    = -9,
    NOT_STARTED     = -10,
};
```

Ces erreurs ne remplacent pas les codes HTTP. Elles servent au contrat d’API interne/public côté bibliothèque.
`TIMEOUT` et `NOT_STARTED` restent réservés en V1 (pas de câblage runtime).

### 6.4 Serveur HTTP

Le serveur doit exposer :

```cpp
class HttpServer {
public:
    explicit HttpServer(uint16_t port,
                        uint8_t max_clients = HTTP_MAX_CLIENTS);

    HttpErr on(HttpMethod        method,
               const char       *path,
               HttpHandler       handler,
               void             *user_ctx = nullptr,
               HttpRouteMatcher  matcher  = nullptr);

    HttpErr serve_static(const char    *url_prefix,
                         const char    *fs_root,
                         HttpFsBackend *backend,
                         const char    *index_file = "index.html");

    HttpErr begin();

    void stop();
};
```

Règles :

- `begin()` initialise la couche HTTP et démarre la couche TCP ;
- `stop()` arrête le serveur et libère l’état actif ;
- `on()` retourne `ROUTE_FULL` si la table de routes est pleine ;
- `on()` retourne `INVALID_ARG` si le handler ou le chemin est invalide ;
- si le paramètre `matcher` passé à `on()` vaut `nullptr`, la bibliothèque utilise automatiquement `http_route_match_exact` ;
- `serve_static()` est compilé uniquement si `HTTP_ENABLE_STATIC_FILES == 1` ;
- `serve_static()` retourne `INVALID_ARG` si le backend ou le préfixe est invalide ;
- `serve_static()` retourne `ROUTE_FULL` si la table de routes statiques ou la table de routes unifiée est pleine.

### 6.5 Handler applicatif

```cpp
typedef void (*HttpHandler)(HttpRequest &req, HttpResponse &res);
```

Règles :

- le handler est appelé après parsing complet de la requête ;
- `req` et `res` sont valides uniquement pendant l’appel ;
- le handler ne doit pas bloquer durablement ;
- le handler doit produire une réponse ou laisser le serveur produire une erreur interne maîtrisée ;
- le handler ne doit pas conserver de pointeurs internes fournis par `HttpRequest`.

### 6.6 Matcher de route

```cpp
typedef bool (*HttpRouteMatcher)(
    const char     *pattern,
    const char     *path,
    HttpRouteMatch *out_match
);
```

En V1, le matcher par défaut est exact :

```cpp
bool http_route_match_exact(
    const char *pattern,
    const char *path,
    HttpRouteMatch *out_match
);
```

Les paramètres dynamiques sont hors périmètre V1, mais l’interface est prévue pour ne pas casser l’API en V2.

## 7. Configuration compile-time

La bibliothèque doit exposer ses paramètres critiques dans `http_config.h`.

| Macro | Défaut V1 | Rôle |
|---|---:|---|
| `HTTP_MAX_ROUTES` | `16` | Nombre maximal de routes |
| `HTTP_MAX_CLIENTS` | `3` | Nombre maximal de clients simultanés |
| `HTTP_LINE_MAX` | `128` | Taille maximale de la request line |
| `HTTP_HEADER_MAX_SIZE` | `512` | Taille totale maximale des headers RX |
| `HTTP_HEADER_MAX_COUNT` | `10` | Nombre maximal de headers parsés |
| `HTTP_BODY_MAX_SIZE` | `1024` | Taille maximale du body |
| `HTTP_BODY_READ_CHUNK` | `128` | Taille de lecture partielle du body |
| `HTTP_QUERY_MAX_SIZE` | `128` | Taille maximale de query string |
| `HTTP_FS_BLOCK_SIZE` | `512` | Taille des blocs fichier |
| `HTTP_RESP_HEADER_MAX` | `6` | Nombre maximal de headers réponse |
| `HTTP_ENABLE_STATIC_FILES` | `1` | Activation fichiers statiques |
| `HTTP_ENABLE_JSON_HELPERS` | `1` | Activation helpers JSON |
| `HTTP_ENABLE_ROUTE_PARAMS` | `0` | Activation future routes dynamiques |
| `HTTP_ROUTE_PARAM_MAX` | `4` | Nombre maximal de paramètres dynamiques |
| `HTTP_PARAM_NAME_MAX` | `24` | Taille nom paramètre dynamique |
| `HTTP_PARAM_VALUE_MAX` | `48` | Taille valeur paramètre dynamique |

Les valeurs par défaut doivent être conservatrices. Modifier ces valeurs revient à modifier le profil mémoire de la bibliothèque.

## 8. Modèle mémoire fonctionnel

### 8.1 Ressources par client actif

Chaque client actif dispose d’un slot fixe.

Les ressources par slot sont statiques :

| Ressource | Taille | Condition |
|---|---:|---|
| Buffer headers RX | `HTTP_HEADER_MAX_SIZE` | Toujours |
| Buffer body RX | `HTTP_BODY_MAX_SIZE` | Toujours |
| Buffer query string | `HTTP_QUERY_MAX_SIZE` | Toujours |
| État parsing HTTP | ~32 o | Toujours |
| État réponse | ~16 o | Toujours |
| État streaming fichier | ~48 o | Si fichiers statiques activés |
| `HttpRouteMatch` minimal | 1 o | Si routes dynamiques désactivées |
| `HttpRouteMatch` dynamique | Configurable | Si routes dynamiques activées |

### 8.2 Estimation RAM V1 par défaut

Pour `HTTP_MAX_CLIENTS = 3` :

| Poste | Calcul | Estimation |
|---|---:|---:|
| Headers RX | 3 × 512 | 1 536 o |
| Body RX | 3 × 1024 | 3 072 o |
| Query string | 3 × 128 | 384 o |
| États internes | 3 × ~64 | ~192 o |
| Streaming fichier | 3 × ~48 | ~144 o |
| Table routes | 16 × ~24 | ~384 o |
| Total estimé | — | ~5,7 Ko |

Cette estimation doit être maintenue dans `docs/memory.md`.

Toute évolution de macro mémoire doit être répercutée dans la documentation.

## 9. Machine d’états attendue

La bibliothèque doit s’appuyer sur une machine d’états par client.

États fonctionnels minimaux :

| État | Rôle |
|---|---|
| `IDLE` | Slot libre ou connexion initialisée |
| `READ_LINE` | Lecture de la ligne de requête |
| `READ_HEADERS` | Accumulation et parsing des headers |
| `READ_BODY` | Réception du body borné |
| `ROUTE` | Résolution route applicative ou fichier statique |
| `CALL_HANDLER` | Exécution handler utilisateur |
| `SEND_RESPONSE` | Émission réponse courte |
| `SEND_FILE` | Streaming fichier statique |
| `CLOSING` | Fermeture après vidage TX |
| `ERROR` | Erreur maîtrisée, avec réponse HTTP courte si possible, puis fermeture propre |

Règles de transition vers `ERROR` :

- `ERROR` peut être atteint depuis `READ_LINE`, `READ_HEADERS`, `READ_BODY`, `ROUTE`, `CALL_HANDLER`, `SEND_RESPONSE` ou `SEND_FILE` ;
- si les headers ne sont pas encore complets, aucune réponse HTTP n’est requise et la connexion est fermée proprement ;
- si la requête est suffisamment formée et que le buffer TX accepte une réponse, `ERROR` génère une réponse courte adaptée (`400`, `413` ou `500`) ;
- après planification d’une réponse d’erreur, l’état suivant est `CLOSING` ;
- si aucune réponse ne peut être planifiée, la connexion est fermée proprement sans réponse ;
- si un fichier est ouvert au moment de l’erreur, son handle doit être fermé avant libération du slot ;
- `ERROR` ne doit jamais rappeler le handler applicatif et ne doit jamais boucler.

La libération du slot client passe par le même chemin de nettoyage que les fermetures normales afin d’éviter les divergences d’état entre erreur, timeout et fermeture standard.

Les états exacts peuvent différer dans l’implémentation, mais le comportement observable doit rester équivalent.

## 10. Gestion des timeouts

En V1.x, la couche HTTP ne maintient pas de timeout par phase.

Le timeout de connexion est délégué à `esp8266-tcp-transport` via
`TCP_IDLE_TIMEOUT_MS`. La valeur par défaut du transport est `0`, donc le
timeout est désactivé sauf override projet.

Les timeouts header/body avec suivi d'état HTTP et réponse `408 Request Timeout`
sont réservés à une version ultérieure.

## 11. Robustesse et erreurs

La bibliothèque doit gérer sans corruption :

- requête invalide ;
- méthode inconnue ;
- ligne de requête trop longue ;
- headers trop longs ;
- trop grand nombre de headers ;
- `Content-Length` invalide ;
- body trop gros ;
- timeout de connexion si activé côté transport ;
- client silencieux ;
- client déconnecté ;
- buffer TX saturé ;
- route absente ;
- méthode non autorisée ;
- erreur fichier ;
- chemin fichier invalide ;
- tentative d’envoi multiple.

Chaque erreur doit produire soit :

- une réponse HTTP courte si possible ;
- une fermeture propre sans réponse si la requête est incomplète ;
- une erreur API explicite si l’appel provient de l’application.

## 12. Sécurité minimale

La V1 ne fournit pas de sécurité applicative complète.

Elle doit néanmoins empêcher :

- path traversal dans les fichiers statiques ;
- dépassement de buffers ;
- body non borné ;
- consommation excessive de slots par clients silencieux ;
- interprétation incohérente des headers critiques ;
- envoi multiple non contrôlé ;
- dépendance implicite à des allocations dynamiques.

La bibliothèque n’implémente pas :

- authentification ;
- session ;
- CSRF ;
- TLS natif ;
- contrôle d’accès applicatif.

Ces sujets relèvent d’évolutions futures ou de la couche application.

## 13. Architecture logicielle cible

### 13.1 Organisation fonctionnelle

Structure recommandée du dépôt :

```text
include/
  http_server.h
  http_request.h
  http_response.h
  http_fs_backend.h
  http_config.h

src/
  http_server.cpp
  http_engine.cpp
  http_engine.h
  http_parser.cpp
  http_parser.h
  http_router.cpp
  http_router.h
  http_response.cpp
  http_request.cpp
  http_static_files.cpp
  http_static_files.h
  http_tcp_adapter.cpp
  http_tcp_adapter.h
  http_url_decode.cpp
  http_url_decode.h

examples/
  minimal/
  json_api/
  static_files_littlefs/

docs/
  api.md
  architecture.md
  memory.md
  static-files.md
  tests.md

AGENTS.md
README.md
README.en.md
```

### 13.2 Rôle des modules

| Module | Responsabilité |
|---|---|
| `http_server.*` | API publique serveur, enregistrement routes, cycle de vie |
| `http_engine.*` | Machine d’états HTTP par client |
| `http_parser.*` | Parsing request line, headers, body |
| `http_router.*` | Résolution routes et méthodes |
| `http_request.*` | Vue applicative de la requête |
| `http_response.*` | Construction réponse HTTP |
| `http_static_files.*` | Mapping, sanitisation, streaming fichiers |
| `http_tcp_adapter.*` | Pont entre moteur HTTP et transport TCP |
| `http_url_decode.*` | Décodage query/form minimal |
| `http_config.h` | Macros compile-time |
| `http_fs_backend.h` | Interface backend fichier |

### 13.3 Isolation backend

L’API publique ne doit pas exposer :

- les structures internes du transport TCP ;
- les buffers RX/TX du transport ;
- les handles lwIP ;
- la machine d’états interne ;
- les détails de planification réseau ;
- les structures privées de streaming fichier.

Le remplacement du backend TCP ne doit pas modifier `HttpServer`, `HttpRequest` ou `HttpResponse`.

## 14. Documentation requise

Le dépôt doit contenir :

| Fichier | Contenu attendu |
|---|---|
| `README.md` | Présentation FR, usage minimal, limites V1 |
| `README.en.md` | Version anglaise synthétique |
| `docs/api.md` | Contrat complet API publique |
| `docs/architecture.md` | Architecture interne et séparation des couches |
| `docs/memory.md` | Budget RAM, coût par client, impact macros |
| `docs/static-files.md` | Backend fichier, mapping URL, MIME, sécurité |
| `docs/tests.md` | Plan de tests host et matériel |
| `AGENTS.md` | Cadre de travail Codex, règles de style, interdits |

La documentation doit expliciter les limites plutôt que les masquer.

## 15. Tests et validations

### 15.1 Tests unitaires host

Les tests côté host doivent couvrir :

- parsing `GET` simple ;
- parsing `POST` avec body ;
- parsing `POST` formulaire ;
- extraction query string ;
- décodage `%XX` ;
- décodage `+` en espace ;
- header présent ;
- header absent ;
- dépassement du nombre de headers ;
- ligne de requête trop longue ;
- body trop gros ;
- route exacte trouvée ;
- route absente ;
- méthode non autorisée ;
- path traversal rejeté ;
- double slash rejeté ;
- MIME connu → `Content-Type` attendu selon la table §5.16 ;
- MIME inconnu → `application/octet-stream` ;
- `send()` double ;
- `set_header()` après `send()` ;
- limite headers réponse.

### 15.2 Tests sur matériel ESP8266

Les tests matériels doivent valider :

- démarrage serveur ;
- route `GET /` ;
- route JSON `GET /api/status` ;
- route `POST /api/config` ;
- fichiers statiques ;
- fermeture après réponse ;
- client silencieux ;
- client lent ;
- déconnexion client pendant streaming fichier ;
- plusieurs clients dans la limite configurée ;
- absence de crash après requêtes invalides répétées ;
- stabilité RAM en charge faible ou moyenne.

### 15.3 Critères de non-régression

Une modification ne doit pas :

- introduire Arduino ;
- introduire STL, exceptions ou RTTI ;
- augmenter silencieusement les buffers par défaut ;
- modifier l’ordre de priorité route dynamique / fichier statique ;
- charger les fichiers statiques entièrement en RAM ;
- rendre le keep-alive actif en V1 ;
- exposer le backend TCP dans l’API publique ;
- casser les signatures publiques V1 sans décision explicite.

## 16. Exemples applicatifs requis

### 16.1 Exemple minimal

Doit montrer :

- initialisation Wi-Fi côté application ;
- création `HttpServer` ;
- route `GET /` ;
- réponse HTML ;
- démarrage serveur.

### 16.2 Exemple API JSON

Doit montrer :

- route `GET /api/status` ;
- réponse JSON ;
- Content-Type automatique via `send_json()`.

### 16.3 Exemple configuration POST

Doit montrer :

- route `POST /api/config` ;
- lecture body brut ;
- lecture formulaire via `form_param()` ;
- réponse `204 No Content`.

### 16.4 Exemple fichiers statiques

Doit montrer :

- déclaration d’un backend LittleFS ou équivalent ;
- appel `serve_static()` ;
- accès à `/static/index.html` ;
- fichier envoyé par streaming.

## 17. Critères d’acceptation V1

La V1 est considérée fonctionnelle si :

- le serveur compile sous ESP8266 RTOS SDK avec PlatformIO ;
- aucune dépendance Arduino n’est introduite ;
- le serveur démarre sur ESP8266 ;
- `GET /` retourne une page HTML ;
- `GET /api/status` retourne du JSON ;
- `POST /api/config` lit un petit body ;
- `form_param()` extrait un paramètre formulaire ;
- une route absente retourne `404` ;
- une méthode invalide retourne `405` ;
- un body trop gros retourne `413` ;
- un header trop long est rejeté proprement ;
- un dépassement du nombre de headers retourne `400` ;
- un client silencieux est fermé après timeout ;
- un client lent ne bloque pas les autres slots ;
- `send()` appelé deux fois retourne `HttpErr::ALREADY_SENT` ;
- un chemin statique avec `../` retourne `404` sans appel au backend ;
- un fichier statique existant est servi par blocs ;
- un fichier absent retourne `404` ;
- la fermeture TCP intervient après vidage TX ;
- la RAM consommée est documentée ;
- le nombre de clients reste borné par `HTTP_MAX_CLIENTS` ;
- l’API publique ne dépend pas du backend TCP ;
- le backend fichier est remplaçable ;
- l’implémentation ne duplique pas la couche TCP existante.

## 18. Hors périmètre V1

Sont explicitement exclus :

- TLS natif ;
- WebSocket ;
- Server-Sent Events ;
- HTTP/2 ;
- compression gzip dynamique ;
- upload de gros fichiers ;
- moteur de template HTML ;
- gestion utilisateur ;
- sessions ;
- Wi-Fi manager ;
- DNS captif ;
- OTA ;
- dépendance Arduino ;
- STL ;
- exceptions ;
- RTTI ;
- routes dynamiques `:id` ;
- keep-alive.

Ces éléments ne doivent pas influencer la simplicité du socle V1.

## 19. Évolutions prévues

La conception doit permettre une évolution vers :

- routes dynamiques via `HttpRouteMatcher` ;
- keep-alive optionnel ;
- upload borné ;
- cache-control pour fichiers statiques ;
- authentification basique ;
- middleware simple ;
- métriques serveur ;
- WebSocket léger ;
- SSE ;
- backend TLS séparé ;
- intégration ESP32 ;
- interface Web embarquée plus complète.

Ces évolutions doivent rester compatibles avec l’API publique V1 autant que possible.

## 20. Décision structurante finale

La bibliothèque est un serveur HTTP embarqué minimal, pas un framework Web.

La priorité est :

1. mémoire maîtrisée ;
2. exécution bornée ;
3. API stable ;
4. séparation stricte des couches ;
5. robustesse face aux clients imparfaits ;
6. documentation des limites ;
7. évolutivité sans complexifier la V1.

Le bon résultat n’est pas une bibliothèque riche. Le bon résultat est une bibliothèque prévisible, stable, suffisamment fonctionnelle, et impossible à confondre avec une pile Web généraliste.
