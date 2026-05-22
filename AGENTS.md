# AGENTS.md

## Objectif du dépôt

- Application / service : bibliothèque serveur Web légère pour ESP8266 RTOS SDK.
- Stack : C++ léger, ESP8266 RTOS SDK, PlatformIO, `esp8266-tcp-transport`.
- Architecture générale : API HTTP publique → moteur HTTP borné → adaptateur TCP → `esp8266-tcp-transport` → ESP8266 RTOS SDK / lwIP.

Le dépôt fournit une couche HTTP embarquée minimale : routes, requêtes, réponses, parsing HTTP, fichiers statiques optionnels et intégration TCP. Ce n’est pas un framework Web généraliste.

## Commandes utiles

- Installer : `platformio pkg install`
- Lancer en local : non applicable directement, bibliothèque embarquée.
- Tester : `pio test`
- Linter : à définir selon configuration projet.
- Build : `pio run`

Adapter les commandes si le dépôt définit des environnements PlatformIO spécifiques.

## Règles de modification

- Respecter l’architecture existante.
- Ne pas modifier l’API publique sans justification explicite.
- Ne pas introduire de dépendance sans justification.
- Ne pas introduire Arduino, `ESP8266WebServer` ou `ESPAsyncWebServer`.
- Ne pas introduire STL, exceptions C++ ou RTTI.
- Ne pas ajouter de fonctionnalité hors périmètre V1.
- Ne pas augmenter silencieusement la RAM consommée par client.
- Ne pas exposer le backend TCP dans les headers publics.
- Ne pas charger un fichier statique complet en RAM.
- Préserver la compatibilité avec `esp8266-tcp-transport`.

## Conventions de code

- Style : C++ léger, impératif, lisible, sans surabstraction.
- Nommage : noms explicites, cohérents avec `HttpServer`, `HttpRequest`, `HttpResponse`, `HttpErr`, `HttpMethod`.
- Tests : privilégier les tests host pour parser, routeur, réponse, décodage URL, MIME et sanitisation de chemin.
- Logs : logs courts, désactivables, sans bruit permanent en production.
- Gestion d’erreurs : retour explicite via `HttpErr`, booléen ou code HTTP selon la couche concernée.

Règles fonctionnelles à préserver :

- `matcher == nullptr` dans `on()` active le matcher exact.
- `405 Method Not Allowed` est obligatoire si un chemin existe avec une autre méthode.
- Les routes `on()` priment toujours sur les fichiers statiques.
- Un handler sans `send()` ni `end()` déclenche un `500 Internal Server Error` automatique.
- `send()` appelé deux fois retourne `HttpErr::ALREADY_SENT`.
- `Connection: close` est toujours émis en V1.
- `HttpFsBackend::exists()` est optionnel ; `open()` reste la source de vérité.
- Un chemin statique rejeté ne doit jamais appeler le backend fichier.

## Tests attendus

- Après modification du parser : tests host request line, headers, body, limites et erreurs.
- Après modification du routeur : tests host routes exactes, `404`, `405`, matcher par défaut et table pleine.
- Après modification de `HttpResponse` : tests host headers, `Content-Length`, double `send()`, `end()`, JSON, HTML.
- Après modification fichiers statiques : tests host sanitisation, MIME, `exists()`, `open()`, absence fichier.
- Après modification moteur HTTP : tests avec transport simulé, timeouts, erreurs, fermeture et handler sans réponse.
- Après modification adaptateur TCP : build PlatformIO et test matériel si possible.
- Avant PR : `pio run` + tests disponibles + mention claire des tests non exécutés.

## Zones sensibles

Ne pas toucher sans demande explicite :

- signatures publiques dans `include/` ;
- macros mémoire dans `http_config.h` ;
- modèle de fermeture TCP ;
- machine d’états client ;
- contrat `HttpFsBackend` ;
- ordre de résolution routes `on()` / fichiers statiques / `404` ;
- comportement `405` ;
- gestion `Connection: close` ;
- intégration `esp8266-tcp-transport` ;
- fichiers ou blocs générés automatiquement ;
- secrets, credentials, certificats, tokens, URLs privées.

## Process de réponse attendu

Après intervention, répondre avec :

```text
Fichiers modifiés :
- ...

Changements :
- ...

Tests lancés :
- ...

Non vérifié :
- ...
```

Réponse courte. Pas de résumé global du projet. Pas de proposition hors périmètre. Signaler explicitement tout test non exécuté ou tout comportement non vérifié.

## À éviter

- Documentation exhaustive du projet.
- Dumps d’architecture trop longs.
- Réécriture globale non demandée.
- Dépendances ajoutées par confort.
- Abstractions décoratives.
- Fonctionnalités hors V1.
- Secrets, tokens ou URLs internes sensibles.
- Règles vagues du type « faire du bon code ».
- Instructions contradictoires.
- Contexte historique inutile.
