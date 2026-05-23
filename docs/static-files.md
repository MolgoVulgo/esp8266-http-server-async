# Fichiers statiques

`HttpFsBackend` fournit `open`, `read`, `size`, `close` et optionnellement `exists`.

Règles :

- les routes `on()` priment sur les fichiers statiques ;
- `../`, `..\`, `//`, chemin vide et sortie de racine sont rejetés avant appel backend ;
- si `exists == nullptr`, `open()` teste l'existence ;
- si `exists()` retourne `true` mais `open()` échoue, le fichier est absent ;
- MIME minimal : `.html`, `.css`, `.js`, `.json`, `.ico`, `.png`, `.svg`, `.txt`, `.xml`.

Le moteur lit le backend par blocs dans le buffer TX courant. Les fichiers plus grands que le buffer de réponse courant sont refusés en V1 tant que le streaming multi-cycle via transport réel n'est pas validé.

## Contrainte de taille

En V1, un fichier statique doit tenir dans une réponse HTTP complète :
en-têtes HTTP plus corps. La limite est `HTTP_RESPONSE_BUFFER_SIZE`.

Avec la configuration par défaut, `HTTP_RESPONSE_BUFFER_SIZE` vaut 512 octets
pour rester aligné avec le buffer TX de `esp8266-tcp-transport`. La taille utile
du corps de fichier est donc d'environ `HTTP_RESPONSE_BUFFER_SIZE - 120` octets,
selon la longueur du MIME et des en-têtes.

Si le fichier dépasse cette limite, le moteur ferme le handle backend et retourne
`500 Internal Server Error`.
