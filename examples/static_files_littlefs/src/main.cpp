#include <stddef.h>
#include <stdint.h>
#include "http_server.h"

static HttpServer server(80);

static void *fs_open(void *ctx, const char *path)
{
    (void)ctx;
    (void)path;
    return 0;
}

static size_t fs_read(void *ctx, void *handle, uint8_t *buf, size_t len)
{
    (void)ctx;
    (void)handle;
    (void)buf;
    (void)len;
    return 0;
}

static size_t fs_size(void *ctx, void *handle)
{
    (void)ctx;
    (void)handle;
    return 0;
}

static void fs_close(void *ctx, void *handle)
{
    (void)ctx;
    (void)handle;
}

static HttpFsBackend backend = {
    0,
    fs_open,
    fs_read,
    fs_size,
    fs_close,
    0
};

extern "C" void app_main(void)
{
    server.serve_static("/static", "/www", &backend, "index.html");
    server.begin();
}
