# Plan d’implémentation — Bibliothèque serveur Web ESP8266 RTOS SDK

## 1. Objet du plan

Ce document définit le plan d’implémentation de la bibliothèque serveur Web ESP8266 RTOS SDK.

Il découpe le développement en lots maîtrisables, ordonnés selon les dépendances techniques réelles : configuration, API publique, moteur HTTP, parsing, routage, réponse, intégration TCP, fichiers statiques, tests, documentation.

L’objectif est de produire une V1 stable, bornée en RAM, sans Arduino, sans STL, sans exceptions, sans RTTI, et construite au-dessus de `esp8266-tcp-transport` sans dupliquer la couche TCP.

## 2. Principes directeurs

Le développement doit respecter les priorités suivantes :

1. contrat public stable ;
2. mémoire bornée ;
3. machine d’états explicite ;
4. parsing robuste ;
5. fermeture propre des connexions ;
6. intégration TCP isolée ;
7. fichiers statiques en streaming uniquement ;
8. tests host avant tests matériel ;
9. documentation synchronisée avec les macros et comportements réels.

Aucune fonctionnalité hors V1 ne doit être introduite pendant l’implémentation du socle.

Sont donc interdits pendant la V1 :

- Arduino ;
- STL ;
- exceptions ;
- RTTI ;
- keep-alive ;
- WebSocket ;
- SSE ;
- TLS natif ;
- upload de gros fichiers ;
- moteur de template ;
- routes dynamiques actives ;
- allocation dynamique non maîtrisée.

## 3. Structure cible du dépôt

Structure recommandée :

```text
include/
  http_config.h
  http_types.h
  http_fs_backend.h
  http_request.h
  http_response.h
  http_server.h

src/
  http_url_decode.cpp
  http_url_decode.h
  http_parser.cpp
  http_parser.h
  http_router.cpp
  http_router.h
  http_request.cpp
  http_response.cpp
  http_static_files.cpp
  http_static_files.h
  http_engine.cpp
  http_engine.h
  http_tcp_adapter.cpp
  http_tcp_adapter.h
  http_server.cpp

examples/
  minimal/
  json_api/
  post_config/
  static_files_littlefs/

test/
  test_host/
    test_build.cpp
    test_url_decode.cpp
    test_parser.cpp
    test_router.cpp
    test_request.cpp
    test_response.cpp
    test_static_path.cpp
    test_mime.cpp
    test_engine.cpp
    http_transport_mock.h
    http_transport_mock.cpp

docs/
  api.md
  architecture.md
  manual_test_matrix.md
  memory.md
  static-files.md
  tests.md

AGENTS.md
README.md
README.en.md
library.json
test/platformio.ini
```

**Référence en cas de conflit :** le plan d'implémentation est la référence structurelle pour l'arborescence du dépôt. Le cahier fonctionnel (§13.1) donne une vue simplifiée des couches ; en cas de divergence sur un nom de fichier ou de dossier, le plan prévaut.

## 4. Lot 0 — Verrouillage du contrat V1

### Objectif

Stabiliser les décisions avant d’écrire le moteur.

### Actions

- figer les macros dans `include/http_config.h` ;
- figer les types communs dans `include/http_types.h` ;
- figer les signatures publiques dans :
  - `http_server.h` ;
  - `http_request.h` ;
  - `http_response.h` ;
  - `http_fs_backend.h` ;
- documenter les interdits de build ;
- vérifier que l’API publique ne dépend pas du backend TCP ;
- vérifier que `HttpFsBackend` n’est visible que si `HTTP_ENABLE_STATIC_FILES == 1`.

### Fichiers produits

```text
include/http_config.h
include/http_types.h
include/http_fs_backend.h
include/http_request.h
include/http_response.h
include/http_server.h
test/test_host/test_build.cpp
```

### Critères de sortie

- `test_build.cpp` inclut uniquement les headers publics et compile sans erreur ;
- `test_build.cpp` inclut intentionnellement les headers du backend TCP et vérifie que la compilation échoue (test négatif de dépendance) ;
- aucune inclusion du backend TCP dans les headers publics ;
- aucune STL ;
- aucune exception ;
- aucun RTTI ;
- macros mémoire V1 présentes ;
- signatures alignées avec le cahier fonctionnel.

## 5. Lot 1 — Décodage URL minimal

### Objectif

Implémenter un utilitaire simple, testable côté host, utilisé ensuite par `param()` et `form_param()`.

### Fonctions attendues

- décodage `%XX` ;
- conversion `+` vers espace ;
- copie dans buffer fourni par l’appelant ;
- retour d’échec si buffer insuffisant ;
- aucune allocation dynamique ;
- comportement déterministe sur séquence `%` invalide.

### Fichiers produits

```text
src/http_url_decode.cpp
src/http_url_decode.h
test/test_host/test_url_decode.cpp
```

### Tests requis

- chaîne sans encodage ;
- `%20` vers espace ;
- `+` vers espace ;
- plusieurs paramètres ;
- `%XX` invalide ;
- buffer trop court ;
- sortie null-terminée.

### Critères de sortie

- tous les tests host passent ;
- aucune dépendance ESP8266 ;
- module réutilisable par request/query/form.

## 6. Lot 2 — Parser HTTP host-first

### Objectif

Construire le parser HTTP minimal avant toute intégration réseau.

### Responsabilités

- parser la request line ;
- extraire méthode ;
- extraire chemin ;
- extraire query string ;
- extraire version HTTP ;
- accumuler les headers ;
- détecter la fin des headers ;
- détecter `Content-Length` ;
- détecter `Content-Type` ;
- refuser les dépassements ;
- préparer la réception body.

### États internes parser

```text
PARSER_READ_LINE
PARSER_READ_HEADERS
PARSER_READ_BODY
PARSER_DONE
PARSER_ERROR
```

### Fichiers produits

```text
src/http_parser.cpp
src/http_parser.h
test/test_host/test_parser.cpp
```

### Tests requis

- `GET / HTTP/1.1` ;
- `GET /api/status?x=1 HTTP/1.1` ;
- `POST /api/config HTTP/1.1` avec body ;
- méthode inconnue ;
- ligne trop longue ;
- headers trop longs ;
- trop grand nombre de headers ;
- `Content-Length` absent ;
- `Content-Length` invalide ;
- `Content-Length` supérieur à `HTTP_BODY_MAX_SIZE` ;
- body partiel ;
- body complet ;
- fin headers sans body.

### Critères de sortie

- parsing validé côté host ;
- erreurs classées proprement ;
- aucune réponse HTTP générée par le parser ;
- le parser ne connaît pas le transport TCP.

## 7. Lot 3 — Requête applicative `HttpRequest`

### Objectif

Construire la vue applicative de la requête à partir de l’état parsé.

### Responsabilités

- exposer méthode, path, query, body, body_len ;
- exposer `user_ctx` ;
- exposer `header()` insensible à la casse ;
- exposer `param()` ;
- exposer `form_param()` ;
- exposer `route_param()` avec retour `false` en V1 ;
- garantir l’absence d’allocation ;
- documenter la durée de validité des pointeurs.

### Fichiers produits

```text
src/http_request.cpp
include/http_request.h
test/test_host/test_request.cpp
```

### Tests requis

- header présent ;
- header absent ;
- casse différente sur header ;
- paramètre query présent ;
- paramètre query absent ;
- paramètre query buffer trop court ;
- formulaire URL-encodé valide ;
- formulaire absent ;
- Content-Type incompatible ;
- `route_param()` retourne toujours `false` en V1.

### Décision `HTTP_DEBUG` — invalidation post-callback

La macro `HTTP_DEBUG` est hors périmètre V1. Elle n'est pas définie dans `http_config.h` et n'est pas testée dans ce lot.

Le comportement attendu (invalidation des buffers `path`, `query_raw`, `body` par écriture d'un motif sentinelle après retour du handler) est documenté dans `AGENTS.md` comme règle V2. Aucun code ne doit dépendre de `HTTP_DEBUG` en V1.

### Critères de sortie

- `HttpRequest` ne possède pas les buffers ;
- `HttpRequest` référence uniquement l'état client courant ;
- aucun pointeur interne n'est documenté comme persistant ;
- `test_request.cpp` couvre tous les cas listés ci-dessus.

## 8. Lot 4 — Réponse HTTP `HttpResponse`

### Objectif

Implémenter la construction bornée des réponses courtes.

### Responsabilités

- gérer status code ;
- gérer headers applicatifs ;
- gérer Content-Type ;
- générer Content-Length ;
- générer Connection: close ;
- envoyer texte ;
- envoyer binaire court ;
- envoyer JSON ;
- envoyer HTML ;
- gérer `end()` sans body ;
- empêcher double envoi ;
- empêcher ajout header après envoi.

### Fichiers produits

```text
src/http_response.cpp
include/http_response.h
test/test_host/test_response.cpp
```

### Tests requis

- réponse texte 200 ;
- réponse JSON ;
- réponse HTML ;
- réponse binaire ;
- `end(204)` ;
- `send()` double → `ALREADY_SENT` ;
- `set_header()` après `send()` → `ALREADY_SENT` ;
- limite headers réponse → `HEADER_FULL` ;
- Content-Length correct ;
- Connection: close toujours présent ;
- Content-Type par défaut `text/plain`.

### Critères de sortie

- réponse sérialisable dans buffer TX transport ;
- aucun envoi direct socket ;
- aucune connaissance du routeur ;
- aucune allocation dynamique.

## 9. Lot 5 — Routeur HTTP

### Objectif

Implémenter la résolution des routes applicatives et la décision `404` / `405`.

### Responsabilités

- stocker les routes `on()` ;
- associer méthode + chemin + handler + user_ctx + matcher ;
- utiliser `http_route_match_exact` si matcher `nullptr` ;
- rechercher d’abord méthode + chemin ;
- détecter chemin existant avec autre méthode ;
- produire `405` obligatoire si chemin trouvé avec méthode différente ;
- produire `404` si aucun chemin ne correspond ;
- respecter la priorité routes applicatives avant fichiers statiques ;
- borner la table à `HTTP_MAX_ROUTES`.

### Fichiers produits

```text
src/http_router.cpp
src/http_router.h
test/test_host/test_router.cpp
```

### Tests requis

- route exacte trouvée ;
- méthode différente sur même chemin → `405` ;
- chemin absent → `404` ;
- matcher `nullptr` → matcher exact ;
- matcher personnalisé appelé ;
- table pleine → `ROUTE_FULL` ;
- handler nul → `INVALID_ARG` ;
- path nul ou vide → `INVALID_ARG`.

### Critères de sortie

- routeur indépendant du TCP ;
- routeur indépendant du parser bas niveau ;
- décision `405` obligatoire validée ;
- coût borné par `HTTP_MAX_ROUTES`.

## 10. Lot 6 — Fichiers statiques : chemin, MIME, backend

### Objectif

Implémenter la partie statique hors réseau : mapping URL, sanitisation, MIME, contrat backend.

### Responsabilités

- enregistrer une déclaration statique ;
- mapper `url_prefix` vers `fs_root` ;
- gérer `index_file` ;
- rejeter `../` ;
- rejeter `..\` ;
- rejeter `//` ;
- rejeter chemin vide invalide ;
- empêcher sortie de racine ;
- déterminer MIME minimal ;
- gérer `exists()` facultatif ;
- utiliser `open()` comme source de vérité ;
- retourner absence fichier en `404`.

### Fichiers produits

```text
src/http_static_files.cpp
src/http_static_files.h
test/test_host/test_static_path.cpp
test/test_host/test_mime.cpp
```

### Tests requis

- mapping `/static/a.css` vers `/www/a.css` ;
- `/static/` vers `/www/index.html` ;
- `../` rejeté ;
- `..\` rejeté ;
- `//` rejeté ;
- extension `.html` ;
- extension `.css` ;
- extension `.js` ;
- extension `.json` ;
- extension inconnue → `application/octet-stream` ;
- `exists == nullptr` → test via `open()` ;
- `exists == true` + `open == nullptr` → `404`.

### Critères de sortie

- aucun fichier chargé entièrement en RAM ;
- aucun accès backend sur chemin rejeté ;
- contrat backend stable ;
- logique testée côté host.

## 11. Lot 7 — Moteur HTTP par client

### Objectif

Assembler parser, routeur, requête, réponse et états client dans un moteur HTTP borné.

### États attendus

```text
IDLE
READ_LINE
READ_HEADERS
READ_BODY
ROUTE
CALL_HANDLER
SEND_RESPONSE
SEND_FILE
CLOSING
ERROR
```

### Responsabilités

- gérer un slot client ;
- accumuler RX dans les limites configurées ;
- appeler le parser progressivement ;
- appliquer les timeouts ;
- déclencher le routage ;
- construire `HttpRequest` ;
- construire `HttpResponse` ;
- appeler le handler ;
- générer `500` si handler sans réponse ;
- gérer erreurs parser ;
- gérer `400`, `404`, `405`, `413`, `500` ;
- planifier l’envoi dans le buffer TX ;
- marquer fermeture après réponse ;
- libérer proprement le slot.

### Fichiers produits

```text
src/http_engine.cpp
src/http_engine.h
test/test_host/test_engine.cpp
test/test_host/http_transport_mock.h
test/test_host/http_transport_mock.cpp
```

Le mock transport (`http_transport_mock`) simule les callbacks RX/TX de `esp8266-tcp-transport` sans dépendance réseau. Il doit exposer :

- une fonction d'injection de données RX (simule la réception TCP) ;
- un buffer TX observable (permet de vérifier les réponses générées) ;
- une fonction de signalement de déconnexion client ;
- un compteur de slots alloués / libérés.

Ce mock est défini dans ce lot et réutilisé par le lot 8 (tests adaptateur TCP host, si applicable).

### Tests requis

Tests host possibles avec transport simulé :

- requête complète en un bloc ;
- requête fragmentée ;
- body fragmenté ;
- handler sans réponse → `500` ;
- route absente → `404` ;
- méthode non autorisée → `405` ;
- body trop gros → `413` ;
- header trop long → erreur maîtrisée ;
- fermeture après réponse ;
- libération slot après erreur ;
- pas de rappel handler depuis `ERROR` ;
- handle fichier fermé en erreur `SEND_FILE`.

### Critères de sortie

- moteur indépendant de lwIP ;
- moteur pilotable par événements abstraits ;
- aucun blocage ;
- aucune boucle active ;
- chemins erreur testés.

## 12. Lot 8 — Adaptateur TCP `esp8266-tcp-transport`

### Objectif

Relier le moteur HTTP au transport TCP sans exposer le transport dans l’API publique.

### Responsabilités

- démarrer le serveur TCP via `esp8266-tcp-transport` ;
- associer une connexion TCP à un slot HTTP ;
- transmettre les données RX au moteur ;
- pousser les données TX produites par le moteur vers le transport ;
- recevoir les événements de déconnexion ;
- gérer le manque de slot ;
- gérer le manque de buffer TX ;
- fermer proprement après vidage TX ;
- ne jamais bloquer dans les callbacks transport.

### Fichiers produits

```text
src/http_tcp_adapter.cpp
src/http_tcp_adapter.h
```

### Tests requis

Les comportements de l'adaptateur TCP impliquent `esp8266-tcp-transport` et lwIP. Ils ne peuvent pas être testés côté host sans un stub complet du transport, ce qui dépasse le périmètre V1.

**Décision : les tests de l'adaptateur TCP sont uniquement matériels.** Ils sont couverts par le lot 12 (tests matériels ESP8266) via les scénarios `curl` et les cas réseau listés dans ce lot. Aucun fichier de test host n'est produit dans ce lot.

Les comportements à valider sur matériel :

- connexion acceptée avec slot libre ;
- connexion refusée sans slot (`NO_SLOT`) ;
- données RX transmises au moteur sans perte ;
- données TX générées par le moteur transmises au transport ;
- déconnexion client libère le slot proprement ;
- fermeture connexion déclenchée après vidage TX ;
- saturation buffer TX reportée sans blocage ;
- arrêt serveur via `stop()` ferme les connexions actives.

### Critères de sortie

- API publique inchangée ;
- aucune inclusion transport dans les headers publics ;
- callbacks transport courts ;
- moteur HTTP non dépendant des détails TCP.

## 13. Lot 9 — `HttpServer` public

### Objectif

Assembler API publique, routeur, moteur, fichiers statiques et adaptateur TCP.

### Responsabilités

- constructeur `HttpServer(port, max_clients)` ;
- `on()` ;
- `serve_static()` si activé ;
- `begin()` ;
- `stop()` ;
- validation des arguments ;
- propagation des `HttpErr` ;
- protection contre `begin()` multiple ;
- comportement propre si `stop()` avant `begin()`.

### Fichiers produits

```text
src/http_server.cpp
include/http_server.h
```

### Tests requis

- création serveur ;
- `on()` valide ;
- `on()` invalide ;
- routes pleines ;
- `serve_static()` valide ;
- `serve_static()` backend nul ;
- `serve_static()` table pleine ;
- `begin()` OK ;
- `begin()` erreur transport ;
- `stop()` idempotent.

### Critères de sortie

- API publique utilisable dans un exemple minimal ;
- erreurs retournées explicitement ;
- aucun détail backend exposé.

## 14. Lot 10 — Exemples applicatifs

### Objectif

Fournir des exemples courts servant à la fois de documentation et de validation manuelle.

### Exemples requis

```text
examples/minimal/
examples/json_api/
examples/post_config/
examples/static_files_littlefs/
```

### Exemple minimal

Doit démontrer :

- init Wi-Fi côté application ;
- `HttpServer server(80)` ;
- route `GET /` ;
- `send_html()` ;
- `begin()`.

### Exemple JSON API

Doit démontrer :

- route `GET /api/status` ;
- `send_json()` ;
- réponse courte ;
- absence de dépendance externe.

### Exemple POST config

Doit démontrer :

- route `POST /api/config` ;
- lecture `req.body` ;
- lecture `form_param()` ;
- réponse `204`.

### Exemple fichiers statiques

Doit démontrer :

- backend LittleFS ou équivalent ;
- `serve_static("/static", "/www", &backend)` ;
- index par défaut ;
- fichier servi par blocs.

### Critères de sortie

- exemples compilables ;
- exemples courts ;
- aucun exemple ne contourne l’API publique.

## 15. Lot 11 — Tests host complets

### Objectif

Valider toute la logique indépendante du matériel avant tests ESP8266.

### Modules testés

- URL decode ;
- parser ;
- request ;
- response ;
- router ;
- static path ;
- MIME ;
- moteur avec transport simulé.

### Matrice minimale

| Domaine | Cas attendu |
|---|---|
| Parser GET | chemin, query, headers corrects |
| Parser POST | body complet reconnu |
| Header overflow | erreur contrôlée |
| Header count overflow | `400 Bad Request` côté moteur |
| Body overflow | `413 Payload Too Large` |
| Route exacte | handler appelé |
| Chemin sans méthode | `405 Method Not Allowed` |
| Route absente | `404 Not Found` |
| Handler sans réponse | `500 Internal Server Error` |
| Double send | `ALREADY_SENT` |
| Path traversal | `404`, aucun backend appelé |
| MIME inconnu | `application/octet-stream` |
| exists nullptr | test via `open()` |

### Critères de sortie

- tests host automatisés ;
- pas de dépendance ESP8266 dans les tests host ;
- les erreurs critiques sont couvertes avant flash matériel.

## 16. Lot 12 — Tests matériels ESP8266

### Objectif

Valider le comportement réel sur ESP8266 RTOS SDK avec `esp8266-tcp-transport`.

### Préparation

- firmware exemple minimal ;
- logs série limités ;
- outil client type `curl` ;
- tests réseau depuis LAN ;
- surveillance RAM si disponible ;
- vérification absence reboot watchdog.

### Tests manuels requis

```bash
curl -v http://<ip>/
curl -v http://<ip>/api/status
curl -v -X POST http://<ip>/api/config -d 'name=test'
curl -v http://<ip>/not-found
curl -v -X PUT http://<ip>/
curl -v http://<ip>/static/index.html
curl -v http://<ip>/static/../secret.txt
```

### Cas réseau à valider

- client normal ;
- client lent ;
- client silencieux ;
- body trop gros ;
- plusieurs clients simultanés dans la limite ;
- déconnexion pendant réponse ;
- déconnexion pendant fichier statique ;
- redémarrage serveur après `stop()` si supporté.

### Critères de sortie

- aucun crash ;
- aucune fuite de slot observable ;
- fermeture propre après réponse ;
- RAM compatible avec budget documenté ;
- fichiers statiques envoyés par blocs ;
- pas de blocage task réseau.

## 17. Lot 13 — Documentation

### Objectif

Produire une documentation exploitable pour utilisateur, mainteneur et agent Codex.

### Fichiers requis

```text
README.md
README.en.md
docs/api.md
docs/architecture.md
docs/memory.md
docs/static-files.md
docs/tests.md
AGENTS.md
```

### Contenu attendu

#### `README.md`

- objectif ;
- contraintes ;
- exemple minimal ;
- limites V1 ;
- dépendance `esp8266-tcp-transport`.

#### `README.en.md`

- version anglaise concise ;
- même périmètre ;
- même avertissement sur les limites.

#### `docs/api.md`

- signatures publiques ;
- durée de validité des pointeurs ;
- erreurs `HttpErr` ;
- comportement `send()` ;
- comportement `on()` ;
- comportement `serve_static()`.

#### `docs/architecture.md`

- couches ;
- moteur ;
- parser ;
- routeur ;
- adaptateur TCP ;
- backend fichier.

#### `docs/memory.md`

- macros ;
- coût par client ;
- estimation V1 ;
- impact `HTTP_MAX_CLIENTS` ;
- impact `HTTP_ENABLE_ROUTE_PARAMS`.

#### `docs/static-files.md`

- backend ;
- `exists()` facultatif ;
- sanitisation ;
- MIME ;
- streaming ;
- erreurs.

#### `docs/tests.md`

- tests host ;
- tests matériel ;
- commandes `curl` ;
- critères d’acceptation.

#### `AGENTS.md`

- interdits ;
- style de code ;
- règles mémoire ;
- ordre de modification ;
- obligation de tests ;
- interdiction de modifier l’API publique sans justification.

### Critères de sortie

- documentation alignée avec code ;
- macros mémoire cohérentes ;
- limites V1 visibles ;
- aucun comportement critique non documenté.

## 18. Ordre d’exécution recommandé

Ordre strict :

```text
0. Contrat V1
1. URL decode
2. Parser HTTP
3. HttpRequest
4. HttpResponse
5. Routeur
6. Fichiers statiques hors réseau
7. Moteur HTTP
8. Adaptateur TCP
9. HttpServer public
10. Tests host complets
11. Exemples
12. Tests matériels ESP8266
13. Documentation finale
```

Justification :

- le parser et les utilitaires doivent être testés avant le réseau ;
- le routeur doit être validé avant le moteur ;
- la réponse doit être stable avant l’intégration TCP ;
- les fichiers statiques doivent être validés hors réseau avant streaming ;
- l’adaptateur TCP doit arriver tard pour éviter de masquer les bugs de logique HTTP derrière des problèmes réseau.

## 19. Jalons de validation

### Jalon A — API publique figée

Livrables :

- headers publics ;
- macros ;
- types ;
- signatures.

Validation : compilation headers seuls.

### Jalon B — HTTP host fonctionnel

Livrables :

- parser ;
- request ;
- response ;
- router ;
- tests host.

Validation : requêtes simulées sans ESP8266.

### Jalon C — Moteur HTTP simulé

Livrables :

- machine d’états ;
- gestion erreurs ;
- transport mock.

Validation : scénarios complets host.

### Jalon D — Intégration TCP ESP8266

Livrables :

- adaptateur TCP ;
- serveur minimal ;
- exemple `GET /`.

Validation : `curl` sur matériel.

### Jalon E — V1 complète

Livrables :

- POST ;
- JSON ;
- fichiers statiques ;
- timeouts ;
- erreurs ;
- docs.

Validation : critères d’acceptation V1.

## 20. Risques techniques

| Risque | Impact | Réduction |
|---|---|---|
| Surconsommation RAM | Instabilité ESP8266 | buffers statiques bornés, `HTTP_MAX_CLIENTS = 3` |
| Callbacks TCP trop lourds | blocage réseau | moteur progressif, callbacks courts |
| Parsing incomplet | erreurs HTTP ambiguës | tests host massifs |
| Handler sans réponse | connexion pendante | `500` automatique obligatoire |
| Streaming fichier mal nettoyé | fuite handle / slot | fermeture handle dans `ERROR` et `CLOSING` |
| 405 ambigu | comportement incohérent | parcours obligatoire table routes |
| Backend FS fragile | 404 incohérents | `open()` source de vérité |
| Dépendance TCP qui fuit | API instable | adaptateur isolé |
| Ajout hors périmètre | complexité V1 | AGENTS.md strict |
| Timeouts non testés côté host | comportement timeout validé uniquement sur matériel, bugs détectés tard | mock horloge hors périmètre V1 — timeouts validés uniquement sur matériel (lot 12) ; décision documentée ici |

## 21. Règles de commit recommandées

Chaque lot doit être livré séparément.

Format conseillé :

```text
feat(config): add HTTP V1 compile-time configuration
feat(parser): implement bounded request parser
feat(router): add exact route matching and 405 detection
feat(response): implement bounded HTTP response builder
feat(engine): add per-client HTTP state machine
feat(tcp): integrate esp8266 tcp transport adapter
feat(static): add static file mapping and streaming state

test(parser): cover header and body limits
test(router): cover 404 and 405 decisions
test(static): cover path sanitization and MIME fallback

docs(memory): document V1 RAM budget
```

Chaque commit fonctionnel doit compiler.

Les commits mélangeant API, moteur et documentation doivent être évités sauf correction transversale justifiée.

## 22. Critère final de fin d’implémentation V1

La V1 est terminée lorsque :

- l’API publique compile sans dépendance Arduino ;
- les tests host passent ;
- les exemples compilent ;
- le serveur répond sur ESP8266 ;
- `GET /` fonctionne ;
- `GET /api/status` fonctionne ;
- `POST /api/config` fonctionne ;
- `404`, `405`, `413`, `500` sont validés ;
- les fichiers statiques sont servis par blocs ;
- les timeouts ferment proprement les clients ;
- le nombre de clients reste borné ;
- le budget mémoire est documenté ;
- aucune fonctionnalité hors périmètre V1 n’a été introduite ;
- la documentation décrit fidèlement le comportement réel.

La priorité finale reste la stabilité du socle, pas l’étendue fonctionnelle.
