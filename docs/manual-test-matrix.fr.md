# Matrice de test manuel

Version anglaise prioritaire : [manual-test-matrix.md](manual-test-matrix.md)

Utiliser cette checklist avec le firmware dans `test/hardware_basic` et une
cible ESP8266 reelle.

## Preparation

- Build : `pio run -d test/hardware_basic -e esp12e`
- Upload : `pio run -d test/hardware_basic -e esp12e -t upload`
- Monitor : `pio device monitor -d test/hardware_basic -b 74880`
- Confirmer que l'appareil affiche une adresse IP.

## Verifications HTTP

| Cas | Commande | Attendu |
| --- | --- | --- |
| Route racine | `curl -v http://<ip>/` | `200 OK` |
| API JSON | `curl -v http://<ip>/api/status` | `200 OK`, body JSON |
| Form POST | `curl -v -X POST http://<ip>/api/config -d 'name=test'` | `204 No Content` |
| Route absente | `curl -v http://<ip>/missing` | `404 Not Found` |
| Mauvaise methode | `curl -v -X PUT http://<ip>/` | `405 Method Not Allowed` |
| Body trop grand | POST superieur a `HTTP_BODY_MAX_SIZE` | `413 Payload Too Large` |
| Politique connexion | Toute requete | `Connection: close` |

## Robustesse

- Le client se deconnecte pendant l'envoi du body.
- Le client envoie des headers incomplets puis se deconnecte.
- Plus de `HTTP_MAX_CLIENTS` clients se connectent simultanement.
- Le backend statique retourne une lecture courte.
- Un chemin de traversal comme `/static/../secret.txt` n'appelle pas le backend
  et n'expose aucun fichier.

Noter la version firmware, le commit de la bibliotheque, la carte ESP8266,
l'environnement PlatformIO et les macros HTTP non defaut avec le resultat.
