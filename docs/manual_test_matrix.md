# Tests matériels ESP8266

À valider avec un firmware exemple et `esp8266-tcp-transport` :

- build `pio run -d test/hardware_basic -e esp12e` ;
- upload `pio run -d test/hardware_basic -e esp12e -t upload` ;
- connexion acceptée avec slot libre ;
- connexion refusée sans slot ;
- `GET /` ;
- `GET /api/status` ;
- `POST /api/config` ;
- `404` ;
- `405` ;
- `413` ;
- fermeture après réponse ;
- déconnexion client pendant RX ;
- fichiers statiques et rejet traversal.
