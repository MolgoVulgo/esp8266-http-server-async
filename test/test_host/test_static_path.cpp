#include "test_host.h"

#include "http_static_files.h"

#if HTTP_ENABLE_STATIC_FILES
struct MemFile {
    const char *path;
    const char *data;
};

struct FsCtx {
    MemFile files[2];
    int open_calls;
};

static void *fs_open(void *ctx, const char *path)
{
    FsCtx *fs = reinterpret_cast<FsCtx *>(ctx);
    fs->open_calls++;
    for (int i = 0; i < 2; i++) {
        if (fs->files[i].path != 0 && strcmp(fs->files[i].path, path) == 0) {
            return &fs->files[i];
        }
    }
    return 0;
}

static size_t fs_read(void *, void *handle, uint8_t *buf, size_t len)
{
    MemFile *file = reinterpret_cast<MemFile *>(handle);
    size_t n = strlen(file->data);
    if (n > len) n = len;
    memcpy(buf, file->data, n);
    return n;
}

static size_t fs_size(void *, void *handle)
{
    return strlen(reinterpret_cast<MemFile *>(handle)->data);
}

static void fs_close(void *, void *) {}

static bool fs_exists(void *ctx, const char *path)
{
    FsCtx *fs = reinterpret_cast<FsCtx *>(ctx);
    for (int i = 0; i < 2; i++) {
        if (fs->files[i].path != 0 && strcmp(fs->files[i].path, path) == 0) {
            return true;
        }
    }
    return false;
}
#endif

bool run_test_static_path()
{
#if HTTP_ENABLE_STATIC_FILES
    FsCtx fs;
    HttpFsBackend backend;
    HttpStaticFiles statics;
    HttpStaticResolved out;

    fs.files[0].path = "/www/a.css";
    fs.files[0].data = "body{}";
    fs.files[1].path = "/www/index.html";
    fs.files[1].data = "index";
    fs.open_calls = 0;

    backend.ctx = &fs;
    backend.open = fs_open;
    backend.read = fs_read;
    backend.size = fs_size;
    backend.close = fs_close;
    backend.exists = 0;

    CHECK_EQ_INT(statics.add("/static", "/www", &backend, "index.html"), HttpErr::OK);
    CHECK_TRUE(statics.resolve_open("/static/a.css", &out));
    CHECK_STR(out.fs_path, "/www/a.css");
    backend.close(backend.ctx, out.handle);
    CHECK_TRUE(statics.resolve_open("/static/", &out));
    CHECK_STR(out.fs_path, "/www/index.html");
    backend.close(backend.ctx, out.handle);

    fs.open_calls = 0;
    CHECK_TRUE(!statics.resolve_open("/static/../secret.txt", &out));
    CHECK_EQ_INT(fs.open_calls, 0);
    CHECK_TRUE(!statics.resolve_open("/static/a//b.txt", &out));
    CHECK_EQ_INT(fs.open_calls, 0);

    backend.exists = fs_exists;
    CHECK_TRUE(!statics.resolve_open("/static/missing.txt", &out));
#endif
    return true;
}
