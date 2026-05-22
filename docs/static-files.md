# Fichiers statiques

`HttpFsBackend` fournit `open`, `read`, `size`, `close` et optionnellement `exists`.

Règles :

- les routes `on()` priment sur les fichiers statiques ;
- `../`, `..\`, `//`, chemin vide et sortie de racine sont rejetés avant appel backend ;
- si `exists == nullptr`, `open()` teste l'existence ;
- si `exists()` retourne `true` mais `open()` échoue, le fichier est absent ;
- MIME minimal : `.html`, `.css`, `.js`, `.json`, `.ico`, `.png`, `.svg`, `.txt`, `.xml`.

Le moteur lit le backend par blocs dans le buffer TX courant. Les fichiers plus grands que le buffer de réponse courant sont refusés en V1 tant que le streaming multi-cycle via transport réel n'est pas validé.
