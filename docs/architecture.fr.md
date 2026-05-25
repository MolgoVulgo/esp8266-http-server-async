# Architecture

Version anglaise prioritaire : [architecture.md](architecture.md)

## Couches

```text
Application
  -> HttpServer / HttpRequest / HttpResponse
  -> HttpEngine
  -> HttpParser + HttpRouter + HttpStaticFiles
  -> http_tcp_adapter
  -> esp8266-tcp-transport
  -> ESP8266 RTOS SDK / lwIP
```

## Limites

- Les headers publics sont dans `include/`.
- Le moteur HTTP, le parser, le routeur, le constructeur de reponse et le
  resolveur statique sont dans `src/`.
- L'integration TCP est isolee derriere `src/http_tcp_adapter.*`.
- Les headers publics ne doivent pas inclure ni exposer de types du transport
  TCP.
- Les tests host pilotent `HttpEngine` via
  `test/test_host/http_transport_mock.*`.

## Flux d'une requete

1. L'adaptateur TCP accepte un client et demande un slot a `HttpEngine`.
2. Les octets recus sont fournis a `HttpParser`.
3. Quand le parsing est termine, `HttpRouter` resout les routes applicatives.
4. Si aucune route ne correspond et que la methode est `GET`, `HttpStaticFiles`
   peut resoudre un fichier statique.
5. `HttpResponse` construit la reponse complete dans le buffer TX du slot.
6. L'adaptateur envoie la reponse et ferme la connexion.

## Modele de connexion V1

La V1 utilise un modele strict :

```text
une connexion TCP -> une requete HTTP -> une reponse HTTP -> fermeture
```

Ce modele garde l'utilisation RAM previsible et evite l'etat keep-alive dans le
moteur HTTP.
