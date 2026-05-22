#ifndef HTTP_FS_BACKEND_H
#define HTTP_FS_BACKEND_H

#include <stddef.h>
#include <stdint.h>
#include "http_config.h"

#if HTTP_ENABLE_STATIC_FILES

struct HttpFsBackend {
    void *ctx;
    void *(*open)(void *ctx, const char *path);
    size_t (*read)(void *ctx, void *handle, uint8_t *buf, size_t len);
    size_t (*size)(void *ctx, void *handle);
    void (*close)(void *ctx, void *handle);
    bool (*exists)(void *ctx, const char *path);
};

#endif

#endif
