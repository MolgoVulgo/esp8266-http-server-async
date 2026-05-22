# Mémoire V1

Macros principales dans `include/http_config.h` :

- `HTTP_MAX_CLIENTS`: 3
- `HTTP_MAX_ROUTES`: 16
- `HTTP_LINE_MAX`: 128
- `HTTP_HEADER_MAX_SIZE`: 512
- `HTTP_BODY_MAX_SIZE`: 1024
- `HTTP_QUERY_MAX_SIZE`: 128
- `HTTP_RESPONSE_BUFFER_SIZE`: 1536
- `HTTP_FS_BLOCK_SIZE`: 512

Le coût par client est dominé par le parser, le body RX et le buffer de réponse. Augmenter `HTTP_MAX_CLIENTS`, `HTTP_BODY_MAX_SIZE` ou `HTTP_RESPONSE_BUFFER_SIZE` augmente directement la RAM statique consommée.
