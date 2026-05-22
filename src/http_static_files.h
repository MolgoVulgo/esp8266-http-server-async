#ifndef HTTP_STATIC_FILES_H
#define HTTP_STATIC_FILES_H

#include "http_config.h"

#if HTTP_ENABLE_STATIC_FILES

#include "http_fs_backend.h"
#include "http_types.h"

struct HttpStaticRoute {
    const char *url_prefix;
    const char *fs_root;
    const char *index_file;
    HttpFsBackend *backend;
};

struct HttpStaticResolved {
    const HttpStaticRoute *route;
    char fs_path[HTTP_STATIC_PATH_MAX + 1];
    const char *mime;
    void *handle;
    size_t size;
};

class HttpStaticFiles {
public:
    HttpStaticFiles();

    HttpErr add(const char *url_prefix,
                const char *fs_root,
                HttpFsBackend *backend,
                const char *index_file);

    bool resolve_open(const char *url_path, HttpStaticResolved *out) const;
    uint8_t count() const;

private:
    bool build_path(const HttpStaticRoute *route,
                    const char *url_path,
                    char *out,
                    size_t out_len) const;

    HttpStaticRoute routes_[HTTP_MAX_STATIC_ROUTES];
    uint8_t count_;
};

const char *http_static_mime_type(const char *path);
bool http_static_path_is_safe(const char *relative);

#endif

#endif
