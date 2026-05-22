# Besoins — Bibliothèque serveur Web ESP8266 RTOS SDK

## 1. Objet

Définir les besoins d'une bibliothèque serveur Web pour ESP8266, développée en C++ léger pour `esp8266-rtos-sdk`, sans Arduino, et construite au-dessus de la bibliothèque `esp8266-tcp-transport`.

La bibliothèque doit fournir une couche HTTP légère, bornée et exploitable sur ESP8266, dans l'esprit fonctionnel de `ESPAsyncWebServer`, mais adaptée à un environnement RTOS natif, à faible RAM, sans dépendance Arduino.

## 2. Positionnement

La bibliothèque serveur Web n'est pas une couche TCP.

Elle dépend de la bibliothèque `esp8266-tcp-transport`, qui prend en charge :

- l'écoute TCP ;
- les connexions clientes ;
- les buffers RX/TX ;
- la boucle réseau non bloquante ;
- les callbacks de transport.

La bibliothèque Web prend uniquement en charge :

- l'interprétation HTTP ;
- le routage applicatif ;
- la construction des réponses ;
- la gestion contrôlée des requêtes ;
- l'exposition d'une API publique serveur Web.

## 3. Objectif principal

Fournir un serveur HTTP embarqué utilisable pour :

- interfaces de configuration ;
- pages Web simples ;
- API REST locales ;
- commandes applicatives ;
- supervision minimale ;
- intégration avec d'autres bibliothèques ESP8266 RTOS.

Le serveur doit être fiable, prévisible, limité volontairement, et compatible avec les contraintes fortes de l'ESP8266.

## 4. Contraintes fondamentales

### 4.1 Plateforme cible et langage

- ESP8266 ;
- ESP8266 RTOS SDK ;
- PlatformIO ;
- sans Arduino ;
- sans `ESP8266WebServer` ;
- sans `ESPAsyncWebServer`.

#### Décision langage : C++ léger, API publique C++ uniquement

**La bibliothèque est écrite en C++ léger. La compatibilité C de l'API publique n'est pas garantie en V1.**

Règles strictes :

- Le cœur interne (parsing, état machine, buffers) est écrit en C/C++ impératif, sans abstraction coûteuse.
- L'API publique (`HttpServer`, `HttpRequest`, `HttpResponse`) est exposée en C++ : `struct` avec méthodes membres, `enum class`, références.
- **Interdit en V1 :** STL (`std::string`, `std::vector`, etc.), exceptions (`throw`/`try`/`catch`), RTTI (`typeid`, `dynamic_cast`).
- Les templates sont autorisés uniquement si inline et sans instanciation dynamique.

Justification : « C/C++ » sans décision claire génère des ambiguïtés de build (linkage, ABI, flags de compilation). Le C++ léger sans STL/exceptions est natif dans l'outillage ESP8266 RTOS SDK avec PlatformIO et permet des déclarations d'API nettement plus lisibles qu'un cœur C pur avec wrappers.

### 4.2 Contraintes mémoire

La RAM est la contrainte dominante. L'ESP8266 dispose d'environ 80 Ko de RAM utilisable en pratique, dont une partie est consommée par le SDK et la pile réseau.

La bibliothèque doit :

- limiter strictement le nombre de clients simultanés ;
- éviter les allocations dynamiques non maîtrisées ;
- éviter le chargement complet de gros contenus en RAM ;
- borner la taille des headers HTTP ;
- borner la taille du corps de requête ;
- permettre une configuration compile-time des tailles critiques ;
- rester utilisable avec des buffers TCP déjà statiques.

La conception doit préférer les flux, les petits buffers, les callbacks et les états explicites plutôt que les objets lourds ou les abstractions coûteuses.

### 4.3 Contraintes d'exécution

Le serveur doit :

- ne pas bloquer la task réseau ;
- ne pas exécuter de traitement long dans les callbacks TCP ;
- gérer les requêtes en mode borné ;
- tolérer les clients lents ou incomplets ;
- fermer proprement les connexions invalides ;
- éviter les boucles d'attente active ;
- rester déterministe en charge faible ou moyenne.

## 5. Besoins fonctionnels V1

### 5.1 Démarrage et arrêt serveur

La bibliothèque doit permettre :

- de créer une instance serveur HTTP ;
- de configurer un port d'écoute ;
- de définir un nombre maximal de clients ;
- de démarrer le serveur ;
- d'arrêter le serveur ;
- de libérer ou réinitialiser l'état interne.

Le démarrage HTTP doit encapsuler le démarrage TCP.

L'utilisateur de la bibliothèque ne doit pas manipuler directement le serveur TCP pour un usage HTTP standard.

#### Types publics de base

L'ordre de déclaration suit les dépendances : forward declarations d'abord, types simples ensuite, types composés en dernier.

```cpp
// --- Forward declarations ---
// Nécessaires car HttpHandler référence HttpRequest et HttpResponse,
// et HttpRouteMatcher référence HttpRouteMatch, avant leurs définitions complètes.
struct HttpRequest;
struct HttpResponse;
struct HttpRouteMatch;
struct HttpFsBackend;

// --- Méthode HTTP ---
enum class HttpMethod : uint8_t {
    GET     = 0,
    POST    = 1,
    PUT     = 2,
    DELETE  = 3,
    OPTIONS = 4,
    UNKNOWN = 0xFF
};

// --- Codes d'erreur internes ---
// Retournés par toutes les méthodes API pouvant échouer.
enum class HttpErr : int8_t {
    OK              =  0,
    INVALID_ARG     = -1,   // argument nul ou invalide
    NO_SLOT         = -2,   // plus de slot client disponible
    ROUTE_FULL      = -3,   // table de routes pleine (HTTP_MAX_ROUTES atteint)
    HEADER_FULL     = -4,   // nombre maximal de headers de réponse atteint (HTTP_RESP_HEADER_MAX)
    SEND_FAILED     = -5,   // erreur d'écriture dans le buffer TX transport
    PARSE_ERROR     = -6,   // requête HTTP malformée
    TIMEOUT         = -7,   // timeout header ou body dépassé
    FS_ERROR        = -8,   // erreur d'accès fichier (lecture, ouverture)
    ALREADY_SENT    = -9,   // tentative d'envoi après réponse déjà émise
    NOT_STARTED     = -10,  // serveur non démarré
};

// --- Résultat de matching de route ---
// Défini ici car HttpRouteMatcher en dépend, et HttpRouteMatcher est
// utilisé dans HttpServer::on() avant la définition complète de HttpRequest.
#if HTTP_ENABLE_ROUTE_PARAMS
struct HttpRouteMatch {
    char    keys  [HTTP_ROUTE_PARAM_MAX][HTTP_PARAM_NAME_MAX];
    char    values[HTTP_ROUTE_PARAM_MAX][HTTP_PARAM_VALUE_MAX];
    uint8_t count;
};
#else
struct HttpRouteMatch {
    uint8_t count;  // toujours 0 en V1 exact
};
#endif

// --- Fonction de matching de route ---
// Retourne true si pattern correspond à path.
// out_match peut être nullptr si les paramètres ne sont pas souhaités.
typedef bool (*HttpRouteMatcher)(
    const char     *pattern,
    const char     *path,
    HttpRouteMatch *out_match
);

// --- Callback applicatif ---
// req et res sont valides uniquement pendant la durée de l'appel.
typedef void (*HttpHandler)(HttpRequest &req, HttpResponse &res);
```

#### Priorité des routes

La résolution d'une requête entrante suit l'ordre suivant, indépendamment de l'ordre d'enregistrement dans le code :

1. **Routes exactes enregistrées via `on()`** — vérifiées en premier, dans l'ordre d'enregistrement si plusieurs routes correspondent (ce qui ne peut arriver qu'avec des matchers personnalisés en V2).
2. **Routes de fichiers statiques enregistrées via `serve_static()`** — vérifiées si aucune route `on()` n'a correspondu.
3. **Fallback `404 Not Found`** — si aucune des deux catégories ne correspond.

**Règle explicite :** une route `on()` prime toujours sur `serve_static()`, quel que soit l'ordre d'appel dans le code d'initialisation. Cela permet à l'application de surcharger un chemin de fichier statique avec un handler dynamique sans modifier l'ordre d'enregistrement.

#### Déclaration du serveur

```cpp
class HttpServer {
public:
    explicit HttpServer(uint16_t port, uint8_t max_clients = HTTP_MAX_CLIENTS);

    // Enregistre une route applicative.
    // Si matcher est nullptr, le matcher par défaut http_route_match_exact
    // (strcmp simple) est utilisé automatiquement.
    // Retourne HttpErr::OK ou HttpErr::ROUTE_FULL / HttpErr::INVALID_ARG.
    HttpErr on(HttpMethod        method,
               const char       *path,
               HttpHandler       handler,
               void             *user_ctx = nullptr,
               HttpRouteMatcher  matcher  = nullptr);

    // Déclare un répertoire de fichiers statiques.
    // url_prefix : préfixe URL à intercepter (ex. "/static").
    // fs_root    : racine dans le système de fichiers (ex. "/www").
    // backend    : pointeurs vers les fonctions FS (doit rester valide).
    // index_file : fichier servi si le chemin se termine par '/' (défaut "index.html").
    // Retourne HttpErr::INVALID_ARG si backend est nullptr ou url_prefix vide.
    // Retourne HttpErr::ROUTE_FULL si la table de routes est pleine.
    // Compilé uniquement si HTTP_ENABLE_STATIC_FILES == 1.
    HttpErr serve_static(const char    *url_prefix,
                         const char    *fs_root,
                         HttpFsBackend *backend,
                         const char    *index_file = "index.html");

    // Démarre le serveur (init TCP incluse). Retourne HttpErr::OK ou une erreur.
    HttpErr begin();

    // Arrête le serveur et libère les ressources.
    void stop();
};
```

### 5.2 Parsing HTTP minimal

La bibliothèque doit interpréter au minimum :

- méthode HTTP ;
- chemin ;
- query string ;
- version HTTP ;
- headers ;
- corps de requête borné.

Méthodes minimales :

- `GET` ;
- `POST` ;
- `PUT` ;
- `DELETE` ;
- `OPTIONS`.

Les méthodes non supportées doivent retourner une réponse explicite, typiquement `405 Method Not Allowed`.

### 5.3 Routage

La bibliothèque doit fournir un système de routes permettant :

- d'associer une méthode et un chemin à un callback ;
- de gérer une route exacte ;
- de gérer une route générique ou fallback ;
- de retourner `404 Not Found` si aucune route ne correspond ;
- de distinguer les méthodes pour un même chemin.

#### Fonction de matching abstraite

`HttpRouteMatch` et `HttpRouteMatcher` sont définis en section 5.1 avant `HttpServer`, car `HttpServer::on()` en dépend. Cette section documente uniquement leur comportement et les règles d'extension.

En V1, la bibliothèque fournit un matcher par défaut (`http_route_match_exact`) qui effectue un `strcmp` simple. Il est utilisé automatiquement si l'application ne fournit pas de fonction personnalisée.

```cpp
// Matcher exact fourni par la bibliothèque (comportement V1 par défaut).
bool http_route_match_exact(
    const char *pattern, const char *path, HttpRouteMatch *out_match
);
```

Les paramètres dynamiques de type `/api/item/:id` ne sont pas implémentés en V1. L'interface anticipe leur ajout en V2 sans modifier la signature de `HttpServer::on()` ni celle de `HttpHandler`.

**Coût RAM des paramètres dynamiques :**

Avec `HTTP_ENABLE_ROUTE_PARAMS = 0` (défaut V1), `HttpRouteMatch` ne contient qu'un `uint8_t`. Le coût RAM des tables de paramètres est nul.

Avec `HTTP_ENABLE_ROUTE_PARAMS = 1`, chaque `HttpRouteMatch` coûte environ `HTTP_ROUTE_PARAM_MAX × (HTTP_PARAM_NAME_MAX + HTTP_PARAM_VALUE_MAX)` octets. Ce coût est payé par instance de `HttpRequest` active, donc multiplié par `HTTP_MAX_CLIENTS`. Ce mode ne doit être activé qu'en V2 avec routes dynamiques.

### 5.4 Requête applicative

Le callback applicatif reçoit une référence vers une structure `HttpRequest` exposant les données de la requête HTTP courante.

#### Structure `HttpRequest`

```cpp
struct HttpRequest {

    // --- Champs principaux ---
    // Tous les pointeurs char* pointent dans des buffers internes.
    // Ils sont valides UNIQUEMENT pendant la durée du callback.
    // Ne pas conserver de pointeur après retour du handler.

    HttpMethod   method;       // méthode HTTP parsée (enum class)
    const char  *path;         // chemin URL, null-terminé, sans query string
    const char  *query_raw;    // query string brute ou nullptr si absente
    const char  *body;         // corps de requête ou nullptr si absent
    size_t       body_len;     // taille du corps en octets (0 si absent)
    void        *user_ctx;     // contexte utilisateur fourni à server.on()

    HttpRouteMatch route_match; // paramètres de route (vide en V1 exact)

    // --- Accès aux headers parsés ---
    // Recherche insensible à la casse.
    // Retourne nullptr si le header est absent.
    const char *header(const char *name) const;

    // --- Accès aux paramètres de query string ---
    // Recherche un paramètre dans la query string de l'URL (?name=value).
    // Ne lit PAS le body. À utiliser pour les requêtes GET avec paramètres d'URL.
    // Décode et copie la valeur dans out (taille out_len, null-terminé).
    // Retourne true si le paramètre est trouvé et la valeur copiée.
    // Retourne false si absent ou si out_len insuffisant.
    // Décodage URL minimal : %XX et + → espace pour les caractères courants.
    // Ne conserve aucun buffer interne entre deux appels.
    bool param(const char *name, char *out, size_t out_len) const;

    // --- Accès aux paramètres de formulaire encodés dans le body ---
    // Recherche un paramètre dans le body de la requête au format
    // application/x-www-form-urlencoded (name=value&name2=value2).
    // Applicable typiquement aux requêtes POST de formulaire.
    // Retourne false si Content-Type n'est pas application/x-www-form-urlencoded,
    // si body est absent, si le paramètre est absent, ou si out_len insuffisant.
    // Même décodage URL que param(). Ne conserve aucun buffer interne.
    bool form_param(const char *name, char *out, size_t out_len) const;

    // --- Accès aux paramètres de route dynamique ---
    // Copie la valeur dans out. Même contrat que param().
    // Toujours false en V1 exact (HTTP_ENABLE_ROUTE_PARAMS = 0).
    bool route_param(const char *name, char *out, size_t out_len) const;
};
```

**Règles d'usage :**

- `path`, `query_raw`, `body` sont des pointeurs dans des buffers internes valides uniquement pendant la durée du callback. L'application ne doit pas les stocker.
- Si une copie est nécessaire côté application, elle est à la charge de l'application (`strncpy` dans un buffer local).
- `user_ctx` est passé tel quel, sans copie ni gestion de durée de vie par la bibliothèque.
- `param()` et `route_param()` n'allouent rien. L'application fournit son propre buffer de sortie.

**Note de débogage :** la conservation d'un pointeur `path`, `query_raw` ou `body` au-delà du retour du handler est un bug silencieux difficile à diagnostiquer. En mode debug (`HTTP_DEBUG = 1`), l'implémentation doit invalider ces pointeurs (ex. écriture d'un motif sentinelle dans les buffers) immédiatement après le retour du callback, afin de provoquer une corruption visible plutôt qu'un accès fantôme. Ce mécanisme est documenté dans `AGENTS.md`.

### 5.5 Réponse HTTP

La bibliothèque doit permettre de générer des réponses texte, JSON, HTML, binaires courtes, avec code HTTP et headers personnalisés.

#### Structure `HttpResponse`

```cpp
struct HttpResponse {

    // Définit le code HTTP de la réponse.
    // Doit être appelé AVANT send(). Ignoré après send().
    // Défaut : 200 si non appelé.
    void set_status(int code);

    // Ajoute un header de réponse personnalisé.
    // Doit être appelé AVANT send(). Retourne HttpErr::ALREADY_SENT si appelé après.
    // Borné à HTTP_RESP_HEADER_MAX entrées.
    // Retourne HttpErr::HEADER_FULL si la limite est atteinte (header non ajouté).
    // Retourne HttpErr::INVALID_ARG si name ou value est nullptr.
    HttpErr set_header(const char *name, const char *value);

    // Définit le Content-Type (raccourci de set_header).
    // Doit être appelé AVANT send().
    void set_content_type(const char *ct);

    // Envoie un corps texte null-terminé.
    // Content-Type par défaut : text/plain si non défini.
    // Content-Length est calculé automatiquement (strlen).
    // Retourne HttpErr::ALREADY_SENT si appelé plus d'une fois.
    HttpErr send(const char *body);

    // Envoie un corps binaire de longueur explicite.
    // Content-Length est transmis tel quel.
    // Retourne HttpErr::ALREADY_SENT si appelé plus d'une fois.
    HttpErr send(const uint8_t *data, size_t len);

    // Raccourcis sémantiques (forcent le Content-Type correspondant).
    HttpErr send_json(const char *json);   // application/json
    HttpErr send_html(const char *html);   // text/html; charset=utf-8

    // Réponse sans corps (typiquement 204 No Content).
    // Retourne HttpErr::ALREADY_SENT si une réponse a déjà été émise.
    HttpErr end(int code = 204);
};
```

#### Sémantique stricte de `HttpResponse`

| Cas | Comportement défini |
|---|---|
| `send()` appelé deux fois | Retourne `HttpErr::ALREADY_SENT`. La première réponse est conservée. |
| `set_header()` après `send()` | Retourne `HttpErr::ALREADY_SENT`. Le header n'est pas émis. |
| `set_header()` quand limite atteinte | Retourne `HttpErr::HEADER_FULL`. Le header n'est pas ajouté. |
| `set_status()` après `send()` | Ignoré silencieusement. Le code de la première réponse est conservé. |
| `Content-Length` | Généré automatiquement pour toute réponse avec corps via `send()`. |
| Envoi effectif | Planifié dans le buffer TX de la couche transport. Non immédiat. |
| Fermeture TCP | Déclenchée après vidage du buffer TX par le transport, ou sur erreur réseau. |
| Réponse fichier statique | Via état interne dédié (`HTTP_STATE_SEND_FILE`), jamais via `send()` complet. |

**Note sur l'asymétrie `set_status()` / `set_header()` après `send()` :** `set_status()` ne retourne pas de valeur (`void`) et son appel tardif est ignoré silencieusement, contrairement à `set_header()` qui retourne `HttpErr::ALREADY_SENT`. Cette asymétrie est volontaire : un setter void ne peut pas signaler d'erreur sans surcharge de l'API. Elle est documentée explicitement ici pour éviter toute surprise lors de l'implémentation ou de l'utilisation.

**Formulation de fermeture TCP :**

Après génération complète de la réponse, la connexion est marquée pour fermeture. La fermeture effective intervient lorsque la couche transport a terminé l'envoi des données disponibles dans le buffer TX, ou lorsqu'une erreur réseau survient. La bibliothèque ne ferme jamais la socket de façon brute sans vider le buffer TX.

Codes HTTP minimaux :

- `200 OK` ;
- `204 No Content` ;
- `400 Bad Request` ;
- `404 Not Found` ;
- `405 Method Not Allowed` ;
- `413 Payload Too Large` ;
- `500 Internal Server Error`.

### 5.6 Fichiers statiques

Le serveur doit pouvoir servir des fichiers statiques depuis un système de fichiers embarqué si disponible.

#### Interface d'abstraction backend fichier

La bibliothèque définit une structure `HttpFsBackend` que l'application instancie et fournit au démarrage.

```cpp
struct HttpFsBackend {

    // Ouvre un fichier par chemin absolu (déjà sanitisé par la bibliothèque).
    // Retourne un handle opaque non nul en cas de succès.
    // Retourne nullptr si le fichier est absent ou en cas d'erreur.
    void *(*open)(const char *path);

    // Lit jusqu'à len octets dans buf depuis le handle ouvert.
    // Retourne le nombre d'octets lus (> 0), 0 en fin de fichier, -1 en erreur.
    int (*read)(void *handle, uint8_t *buf, size_t len);

    // Retourne la taille totale du fichier en octets depuis le handle ouvert.
    size_t (*size)(void *handle);

    // Ferme le handle. Doit être appelé même après une erreur de lecture.
    void (*close)(void *handle);

    // Vérifie l'existence d'un fichier SANS l'ouvrir.
    // Optimisation facultative : peut être nullptr.
    // Si nullptr, la bibliothèque utilise open() pour tester l'existence.
    // IMPORTANT : open() est la source de vérité. exists() est une optimisation
    // uniquement. Un exists() == true suivi d'un open() == nullptr est une erreur
    // du backend, pas de la bibliothèque. La bibliothèque gère ce cas en retournant 404.
    bool (*exists)(const char *path);
};
```

#### État interne de streaming fichier

Les fichiers statiques ne sont **jamais** chargés en RAM en une seule fois. La bibliothèque maintient un état interne par client actif en mode envoi fichier.

```cpp
// État interne (non exposé dans l'API publique).
struct HttpFileSendState {
    void   *fs_handle;      // handle ouvert via HttpFsBackend::open()
    size_t  total_size;     // taille totale déclarée via HttpFsBackend::size()
    size_t  bytes_sent;     // octets déjà transmis au buffer TX
    char    mime_type[32];  // type MIME déterminé à l'ouverture
    bool    active;         // true si un envoi fichier est en cours pour ce client
};
```

La machine d'états du client HTTP inclut l'état `HTTP_STATE_SEND_FILE`. Dans cet état :

- la bibliothèque appelle `read()` par blocs de `HTTP_FS_BLOCK_SIZE` octets à chaque itération de la boucle réseau ;
- elle pousse chaque bloc dans le buffer TX du transport ;
- elle avance `bytes_sent` jusqu'à `total_size` ;
- elle appelle `close()` et libère l'état quand `bytes_sent == total_size` ou sur erreur.

**Un handler applicatif ne doit jamais appeler `read()` directement sur le backend fichier.** L'envoi d'un fichier statique est déclenché par la bibliothèque elle-même lors de la résolution d'une route de fichiers statiques, pas dans un callback utilisateur.

#### Sanitisation des chemins

Avant tout accès au backend fichier, la bibliothèque **rejette** les chemins suivants :

- contenant `../` ou `..\\` (path traversal) ;
- contenant `//` (double slash) ;
- chemin vide ou égal à `/` seul sans correspondance de fichier index configuré ;
- chemin dont la résolution sort de la racine statique configurée.

Un chemin rejeté retourne `404 Not Found` sans appel au backend. Aucune exception n'est propagée.

**Comportement attendu :**

- servir un fichier par chemin mappé sur le dossier racine statique configuré ;
- déterminer le type MIME minimal par extension : `.html`, `.css`, `.js`, `.json`, `.ico`, `.png`, `.svg`, `.txt`, `.xml` ;
- pour toute extension non reconnue, utiliser le type MIME de fallback `application/octet-stream` (comportement explicite, non indéfini) ;
- envoyer le contenu par blocs de `HTTP_FS_BLOCK_SIZE` octets ;
- retourner `404 Not Found` si `exists()` retourne false ou si `open()` retourne nullptr.

Si `HTTP_ENABLE_STATIC_FILES` est à 0, la structure `HttpFsBackend` n'est pas compilée.

### 5.7 Corps de requête

La bibliothèque doit accepter un corps de requête borné pour :

- formulaire simple ;
- JSON court ;
- commande locale ;
- payload de configuration.

Limites à prévoir :

- taille maximale globale (`HTTP_BODY_MAX_SIZE`) ;
- taille maximale par lecture (`HTTP_BODY_READ_CHUNK`) ;
- rejet propre avec `413 Payload Too Large` si dépassement ;
- abandon de la requête si timeout body dépassé (voir section 7).

L'upload de gros fichiers n'est pas un besoin V1 prioritaire.

### 5.8 Query string

La bibliothèque doit permettre :

- de lire la query string brute via `HttpRequest::query_raw` ;
- de rechercher un paramètre par nom via `HttpRequest::param()` ;
- de récupérer une valeur courte dans un buffer fourni par l'application ;
- de gérer une absence de paramètre sans erreur (retour `false`).

Le décodage URL est minimal : séquences `%XX` courantes et `+` → espace. Pas de support Unicode complet en V1.

### 5.9 Headers

La bibliothèque doit gérer au minimum :

- `Host` ;
- `Content-Length` ;
- `Content-Type` ;
- `Connection` ;
- `User-Agent` en lecture facultative.

Les headers sont stockés de manière bornée (`HTTP_HEADER_MAX_COUNT` entrées).

**Si le nombre de headers reçus dépasse `HTTP_HEADER_MAX_COUNT`, la requête est rejetée avec `400 Bad Request` et la connexion est fermée proprement.** Ce comportement évite qu'un header critique comme `Content-Length` arrive après la limite et soit silencieusement ignoré, entraînant une mauvaise interprétation du body.

**Politique de stockage des headers inconnus (décision figée en V1) :** tous les headers reçus consomment un slot dans la limite `HTTP_HEADER_MAX_COUNT`, qu'ils soient reconnus ou non. La bibliothèque ne filtre pas sur liste blanche en réception. Seuls les headers utiles (liste minimale ci-dessus) sont exposés via `HttpRequest::header()` ; les autres sont stockés mais non accessibles applicativement. Ce comportement simple est préféré à un filtrage sélectif qui créerait un risque de comptage incohérent entre headers critiques reçus et limite atteinte.

### 5.10 Connexions

**En V1, le modèle de connexion est : une requête → une réponse → fermeture.**

Ce comportement est non configurable en V1 et s'applique à toutes les connexions sans exception.

La bibliothèque envoie systématiquement `Connection: close` dans chaque réponse HTTP.

Après génération complète de la réponse, la connexion est marquée pour fermeture. La fermeture effective intervient après vidage du buffer TX par le transport (voir section 5.5).

**Justification :** le keep-alive impose une gestion d'état par connexion (timeout d'inactivité, compteur de requêtes, réutilisation de buffer) incompatible avec les contraintes mémoire et la simplicité du modèle V1. Le surcroît de TCP handshake est négligeable pour un serveur de configuration locale.

Le keep-alive reste prévu comme évolution V2 (section 12).

## 5A. Modèle mémoire par client

Cette section est normative. Elle définit la RAM consommée par client HTTP actif et permet d'estimer le budget mémoire total.

### Ressources allouées par client actif

Chaque slot client actif possède les ressources suivantes, **toutes statiques** (pas d'allocation dynamique) :

| Ressource | Taille | Condition |
|---|---|---|
| Buffer header RX | `HTTP_HEADER_MAX_SIZE` | Toujours présent |
| Buffer body RX | `HTTP_BODY_MAX_SIZE` | Alloué statiquement, utilisé si body présent |
| Buffer query string | `HTTP_QUERY_MAX_SIZE` | Toujours présent |
| État de parsing HTTP | ~32 octets | Toujours présent |
| État de réponse | ~16 octets | Toujours présent |
| État fichier streaming | ~`sizeof(HttpFileSendState)` | Si `HTTP_ENABLE_STATIC_FILES = 1` |
| `HttpRouteMatch` | 1 octet (`count` seul) | Si `HTTP_ENABLE_ROUTE_PARAMS = 0` |
| `HttpRouteMatch` avec params | `HTTP_ROUTE_PARAM_MAX × (NAME + VALUE)` | Si `HTTP_ENABLE_ROUTE_PARAMS = 1` |

**Les buffers sont par client, pas partagés.** Un body buffer de 2048 octets pour 3 clients représente 6 Ko statiques réservés, indépendamment du fait que les requêtes contiennent un body ou non.

### Estimation RAM totale (valeurs par défaut V1)

| Poste | Calcul | Total |
|---|---|---|
| Headers RX (3 clients) | 3 × 512 | 1 536 o |
| Body RX (3 clients) | 3 × 1024 | 3 072 o |
| Query string (3 clients) | 3 × 128 | 384 o |
| États internes (3 clients) | 3 × ~64 | ~192 o |
| Fichier streaming (3 clients) | 3 × ~48 | ~144 o |
| Table de routes | 16 × ~24 | ~384 o |
| **Total estimé** | | **~5,7 Ko** |

Cette estimation correspond aux valeurs par défaut révisées (section 7). Elle doit être documentée dans le dépôt et mise à jour si les macros changent.

## 6. Besoins non fonctionnels

### 6.1 API publique stable

L'API publique doit rester indépendante :

- du backend TCP ;
- du système de fichiers ;
- des choix internes de parsing ;
- des tailles exactes de buffers ;
- de l'organisation des tasks.

Le contrat public doit pouvoir survivre à une évolution interne du backend.

### 6.2 Isolation backend

La bibliothèque doit distinguer clairement :

- API publique HTTP (`http_server.h`) ;
- moteur HTTP interne (`http_engine.cpp` / `http_engine.h`) ;
- adaptateur TCP (`http_tcp_adapter.cpp` / `http_tcp_adapter.h`) ;
- adaptateur fichier (`HttpFsBackend`, déclaré dans `http_fs_backend.h`) ;
- configuration compile-time (`http_config.h`) ;
- exemples applicatifs (`examples/`).

Les fichiers internes sont en `.cpp` / `.h`, cohérent avec le choix C++ léger (section 4.1). Si une unité interne est écrite en C pur pour des raisons de portabilité ou d'interopérabilité (ex. parsing bas niveau), elle porte l'extension `.c` / `.h` et est compilée avec les flags C appropriés — cela doit être explicitement documenté dans le `CMakeLists.txt` ou `platformio.ini`.

Le code applicatif ne doit pas dépendre directement des détails de `http_tcp_adapter.cpp`.

### 6.3 Robustesse

Le serveur doit gérer proprement :

- requête invalide ;
- header trop long ;
- body trop gros ;
- client déconnecté ;
- client silencieux ;
- route absente ;
- méthode inconnue ;
- manque de buffer TX ;
- erreur de lecture fichier ;
- chemin de fichier invalide ou traversal.

Une erreur HTTP ne doit pas corrompre l'état serveur ni l'état des autres clients actifs.

### 6.4 Prévisibilité

Le comportement doit être explicite et borné.

Pas de magie.

Pas d'allocation implicite massive.

Pas de traitement caché coûteux.

Pas de dépendance à une boucle Arduino.

### 6.5 Portabilité interne

Même si la cible est l'ESP8266 RTOS SDK, la séparation logique doit permettre à terme :

- un backend TCP différent ;
- un backend fichier différent ;
- une extension vers ESP32 ;
- une évolution vers TLS via proxy ou backend dédié.

Cette portabilité ne doit pas alourdir la V1.

## 7. Besoins de configuration

La bibliothèque expose des paramètres de configuration sous forme de macros compile-time dans `http_config.h`.

| Macro | Valeur par défaut | Description |
|---|---|---|
| `HTTP_MAX_ROUTES` | `16` | Nombre maximal de routes enregistrées |
| `HTTP_MAX_CLIENTS` | `3` | Nombre maximal de connexions simultanées |
| `HTTP_LINE_MAX` | `128` | Taille maximale de la request line |
| `HTTP_HEADER_MAX_SIZE` | `512` | Taille totale maximale des headers en réception |
| `HTTP_HEADER_MAX_COUNT` | `10` | Nombre maximal de headers parsés par requête |
| `HTTP_BODY_MAX_SIZE` | `1024` | Taille maximale du corps de requête |
| `HTTP_BODY_READ_CHUNK` | `128` | Taille d'une lecture partielle de body |
| `HTTP_QUERY_MAX_SIZE` | `128` | Taille maximale de la query string |
| `HTTP_FS_BLOCK_SIZE` | `512` | Taille des blocs d'envoi pour les fichiers statiques |
| `HTTP_TIMEOUT_HEADER_MS` | `3000` | Timeout pour recevoir les headers complets (ms) |
| `HTTP_TIMEOUT_BODY_MS` | `5000` | Timeout pour recevoir le body complet (ms) |
| `HTTP_TIMEOUT_IDLE_MS` | `5000` | Timeout avant fermeture d'un client silencieux (ms) |
| `HTTP_RESP_HEADER_MAX` | `6` | Nombre maximal de headers dans une réponse |
| `HTTP_ENABLE_STATIC_FILES` | `1` | Active ou désactive les fichiers statiques |
| `HTTP_ENABLE_JSON_HELPERS` | `1` | Active ou désactive les helpers JSON |
| `HTTP_ENABLE_ROUTE_PARAMS` | `0` | Active les paramètres dynamiques de route (V2) |
| `HTTP_ROUTE_PARAM_MAX` | `4` | Nombre maximal de paramètres dynamiques par route |
| `HTTP_PARAM_NAME_MAX` | `24` | Taille maximale d'un nom de paramètre de route |
| `HTTP_PARAM_VALUE_MAX` | `48` | Taille maximale d'une valeur de paramètre de route |

**Notes sur les valeurs par défaut :**

- `HTTP_MAX_CLIENTS = 3` : au-delà de 3 clients simultanés avec buffers statiques par client, la RAM consommée dépasse confortablement 8 Ko rien pour la couche HTTP, ce qui est excessif pour un serveur de configuration locale.
- `HTTP_TIMEOUT_IDLE_MS = 5000` : réduit de 10 000 à 5 000 ms. Un slot occupé 10 secondes par un client silencieux bloque un tiers des slots disponibles.
- `HTTP_ENABLE_ROUTE_PARAMS = 0` : désactivé par défaut en V1. L'activer sans matcher dynamique ne sert à rien et coûte de la RAM.

**Règles sur les timeouts :**

- `HTTP_TIMEOUT_HEADER_MS` démarre dès l'ouverture de la connexion et s'applique jusqu'à réception de la ligne vide séparant headers et body.
- `HTTP_TIMEOUT_BODY_MS` démarre dès la fin des headers et s'applique jusqu'à réception complète du body (`Content-Length` octets reçus).
- `HTTP_TIMEOUT_IDLE_MS` s'applique à un client connecté qui n'envoie rien. Si ce timeout expire avant que la request line ait été reçue, la connexion est fermée sans réponse HTTP.
- Un timeout dépassé entraîne la fermeture propre de la connexion et la libération du slot client. Aucune réponse HTTP n'est émise si les headers n'ont pas encore été reçus.

La configuration peut être complétée par une structure `HttpServerConfig` passée au démarrage pour les paramètres dynamiques (port, nombre de clients effectif).

## 8. Besoins d'API utilisateur

L'utilisateur de la bibliothèque doit pouvoir écrire une application simple avec :

- initialisation Wi-Fi côté application ;
- création du serveur HTTP ;
- déclaration de routes via `server.on()` ;
- callbacks applicatifs (`HttpHandler`) ;
- envoi de réponses via `HttpResponse` ;
- démarrage serveur via `server.begin()`.

#### Exemple d'usage minimal attendu

```cpp
HttpServer server(80);

void handle_root(HttpRequest &req, HttpResponse &res) {
    res.send_html("<h1>OK</h1>");
}

void handle_status(HttpRequest &req, HttpResponse &res) {
    res.send_json("{\"status\":\"ok\"}");
}

void handle_config(HttpRequest &req, HttpResponse &res) {
    // body brut disponible via req.body / req.body_len (JSON, binaire, etc.)
    // formulaire URL-encodé lisible via form_param()
    char val[64];
    if (req.form_param("name", val, sizeof(val))) {
        // utiliser val extrait du body application/x-www-form-urlencoded
    } else if (req.body && req.body_len > 0) {
        // traiter req.body comme JSON ou payload brut
    }
    res.end(204);
}

void app_main() {
    // ... init wifi ...
    server.on(HttpMethod::GET,  "/",           handle_root);
    server.on(HttpMethod::GET,  "/api/status", handle_status);
    server.on(HttpMethod::POST, "/api/config", handle_config);

    // Fichiers statiques : requêtes /static/* → fichiers dans /www/* sur le FS
    // (HTTP_ENABLE_STATIC_FILES doit être à 1)
    server.serve_static("/static", "/www", &my_littlefs_backend);

    server.begin();
}
```

L'API ne doit pas exposer plus de concepts que nécessaire.

Concepts publics attendus :

- serveur (`HttpServer`) ;
- handler (`HttpHandler`) ;
- matcher (`HttpRouteMatcher`) ;
- requête (`HttpRequest`) ;
- réponse (`HttpResponse`) ;
- contexte utilisateur (`void *user_ctx`) ;
- codes d'erreur (`HttpErr`) ;
- méthode HTTP (`HttpMethod`) ;
- backend fichier (`HttpFsBackend`) ;
- déclaration statique (`HttpServer::serve_static()`).

## 9. Besoins de documentation

Le dépôt devra contenir :

- `README.md` en français et anglais ;
- documentation d'API (`docs/api.md`) ;
- documentation mémoire avec tableau par client (`docs/memory.md`) ;
- documentation architecture (`docs/architecture.md`) ;
- exemple minimal ;
- exemple API JSON ;
- exemple fichiers statiques avec backend LittleFS ;
- limites connues ;
- choix de conception ;
- règles pour Codex dans `AGENTS.md`.

La documentation doit expliciter les limites au lieu de les masquer.

## 10. Besoins de tests

La bibliothèque doit prévoir des tests ou validations sur :

- parsing de requête GET ;
- parsing de requête POST avec body brut (`req.body` / `req.body_len`) ;
- parsing POST formulaire via `form_param()` ;
- route trouvée (exact) ;
- route absente → 404 ;
- méthode non autorisée → 405 ;
- header trop long → fermeture propre ;
- dépassement `HTTP_HEADER_MAX_COUNT` → 400 Bad Request ;
- body trop gros → 413 ;
- timeout header dépassé → fermeture sans réponse ;
- timeout body dépassé → fermeture sans réponse ;
- client silencieux → fermeture après `HTTP_TIMEOUT_IDLE_MS` ;
- `send()` appelé deux fois → `HttpErr::ALREADY_SENT` ;
- `param()` trouvé et copié correctement depuis query string ;
- `form_param()` trouvé et copié depuis body URL-encodé ;
- `param()` absent → retour false sans crash ;
- chemin avec `../` → 404 sans accès FS ;
- `serve_static()` retourne 404 si fichier absent ;
- `serve_static()` envoie un fichier par blocs sans le charger en RAM ;
- client qui coupe la connexion en cours d'envoi ;
- plusieurs clients simultanés dans la limite configurée.

Les tests unitaires s'exécutent côté host (parsing, routing, sanitisation chemin, `param()`).

Les tests réseau se valident sur matériel ESP8266.

## 11. Hors périmètre V1

Ne fait pas partie du besoin V1 :

- TLS natif ;
- WebSocket ;
- Server-Sent Events ;
- HTTP/2 ;
- compression gzip dynamique ;
- upload de gros fichiers ;
- moteur de template HTML ;
- gestion utilisateur ou session ;
- Wi-Fi manager ;
- DNS captif ;
- OTA ;
- dépendance Arduino ;
- STL, exceptions ou RTTI C++ ;
- routes dynamiques avec paramètres (`:id`) ;
- keep-alive.

Ces éléments peuvent exister en extensions futures, mais ne doivent pas influencer la structure minimale de départ.

## 12. Besoins d'évolution

La bibliothèque doit pouvoir évoluer vers :

- fichiers statiques plus avancés (index, cache-control) ;
- upload borné ;
- WebSocket léger ;
- SSE ;
- authentification basique ;
- middleware simple ;
- routes dynamiques via matcher `:id` (infrastructure `HttpRouteMatcher` déjà prévue en V1) ;
- keep-alive optionnel avec timeout par connexion ;
- backend TLS séparé ;
- métriques serveur ;
- intégration avec une interface Web embarquée.

L'évolution doit se faire sans casser l'API publique V1 sauf nécessité explicite.

## 13. Critères d'acceptation V1

La V1 est considérée utilisable si :

- le serveur démarre sur ESP8266 RTOS SDK ;
- une route `GET /` retourne une page HTML ;
- une route `GET /api/status` retourne du JSON ;
- une route `POST /api/config` lit un petit body via `req.body` / `req.body_len` ou `form_param()` ;
- une route absente retourne `404` ;
- une méthode invalide retourne `405` ;
- un client lent ne bloque pas le serveur ;
- un client silencieux est fermé après `HTTP_TIMEOUT_IDLE_MS` ;
- `send()` appelé deux fois retourne `HttpErr::ALREADY_SENT` sans crasher ;
- un chemin avec `../` retourne `404` sans accès FS ;
- plusieurs clients restent bornés par `HTTP_MAX_CLIENTS` ;
- la RAM consommée est documentée conformément à la section 5A ;
- aucune dépendance Arduino n'est introduite ;
- la bibliothèque utilise `esp8266-tcp-transport` sans dupliquer la couche TCP ;
- le backend FS est remplaçable sans modifier l'API publique ;
- `serve_static()` avec un backend valide sert un fichier existant sans le charger en RAM.

## 14. Décision structurante

La bibliothèque est pensée comme une couche HTTP embarquée, pas comme une copie de `ESPAsyncWebServer`.

Le modèle en couches :

```text
Application
  ↓
API serveur Web (HttpServer / HttpRequest / HttpResponse / HttpHandler)
  ↓
Moteur HTTP borné (parsing, routing, HttpRouteMatcher, machine d'états client)
  ↓
Adaptateur TCP (http_tcp_adapter)
  ↓
esp8266-tcp-transport
  ↓
ESP8266 RTOS SDK / lwIP
```

Le backend fichier est une branche latérale, instanciée uniquement si `HTTP_ENABLE_STATIC_FILES = 1` :

```text
Moteur HTTP borné (HTTP_STATE_SEND_FILE)
  ↓
HttpFsBackend (struct de callbacks, sanitisation chemin incluse)
  ↓
LittleFS / SPIFFS / autre FS (choix applicatif)
```

La priorité est la maîtrise : mémoire, exécution, API, dépendances.

Les fonctionnalités viennent après la stabilité du socle.
