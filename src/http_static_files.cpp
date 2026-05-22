#include "http_static_files.h"

#if HTTP_ENABLE_STATIC_FILES

#include <string.h>

static bool starts_with(const char *s, const char *prefix)
{
    size_t n;
    if (s == 0 || prefix == 0) {
        return false;
    }
    n = strlen(prefix);
    if (strncmp(s, prefix, n) != 0) {
        return false;
    }
    return s[n] == '\0' || s[n] == '/';
}

HttpStaticFiles::HttpStaticFiles()
{
    count_ = 0;
}

HttpErr HttpStaticFiles::add(const char *url_prefix,
                             const char *fs_root,
                             HttpFsBackend *backend,
                             const char *index_file)
{
    if (url_prefix == 0 || fs_root == 0 || backend == 0 ||
        backend->open == 0 || backend->read == 0 ||
        backend->size == 0 || backend->close == 0 ||
        url_prefix[0] != '/' || fs_root[0] == '\0') {
        return HttpErr::INVALID_ARG;
    }
    if (count_ >= HTTP_MAX_STATIC_ROUTES) {
        return HttpErr::ROUTE_FULL;
    }
    routes_[count_].url_prefix = url_prefix;
    routes_[count_].fs_root = fs_root;
    routes_[count_].backend = backend;
    routes_[count_].index_file = index_file;
    count_++;
    return HttpErr::OK;
}

bool http_static_path_is_safe(const char *relative)
{
    if (relative == 0 || relative[0] == '\0') {
        return false;
    }
    if (strstr(relative, "../") != 0 ||
        strstr(relative, "..\\") != 0 ||
        strstr(relative, "//") != 0 ||
        strstr(relative, "\\\\") != 0) {
        return false;
    }
    if (strcmp(relative, "..") == 0 || strstr(relative, "/..") != 0) {
        return false;
    }
    return true;
}

bool HttpStaticFiles::build_path(const HttpStaticRoute *route,
                                 const char *url_path,
                                 char *out,
                                 size_t out_len) const
{
    size_t prefix_len;
    const char *relative;
    bool need_slash;

    if (route == 0 || url_path == 0 || out == 0 || out_len == 0 ||
        !starts_with(url_path, route->url_prefix)) {
        return false;
    }

    prefix_len = strlen(route->url_prefix);
    relative = url_path + prefix_len;
    if (relative[0] == '/') {
        relative++;
    }

    if (relative[0] == '\0') {
        if (route->index_file == 0 || route->index_file[0] == '\0') {
            return false;
        }
        relative = route->index_file;
    }

    if (!http_static_path_is_safe(relative)) {
        return false;
    }

    need_slash = route->fs_root[strlen(route->fs_root) - 1] != '/';
    if (strlen(route->fs_root) + (need_slash ? 1 : 0) + strlen(relative) >= out_len) {
        return false;
    }

    strcpy(out, route->fs_root);
    if (need_slash) {
        strcat(out, "/");
    }
    strcat(out, relative);
    return true;
}

bool HttpStaticFiles::resolve_open(const char *url_path, HttpStaticResolved *out) const
{
    if (out == 0) {
        return false;
    }
    out->route = 0;
    out->fs_path[0] = '\0';
    out->mime = "application/octet-stream";
    out->handle = 0;
    out->size = 0;

    for (uint8_t i = 0; i < count_; i++) {
        if (!build_path(&routes_[i], url_path, out->fs_path, sizeof(out->fs_path))) {
            continue;
        }

        if (routes_[i].backend->exists != 0 &&
            !routes_[i].backend->exists(routes_[i].backend->ctx, out->fs_path)) {
            return false;
        }

        out->handle = routes_[i].backend->open(routes_[i].backend->ctx, out->fs_path);
        if (out->handle == 0) {
            return false;
        }
        out->route = &routes_[i];
        out->mime = http_static_mime_type(out->fs_path);
        out->size = routes_[i].backend->size(routes_[i].backend->ctx, out->handle);
        return true;
    }

    return false;
}

uint8_t HttpStaticFiles::count() const
{
    return count_;
}

const char *http_static_mime_type(const char *path)
{
    const char *dot;
    if (path == 0) {
        return "application/octet-stream";
    }
    dot = strrchr(path, '.');
    if (dot == 0) {
        return "application/octet-stream";
    }
    if (strcmp(dot, ".html") == 0) return "text/html; charset=utf-8";
    if (strcmp(dot, ".css") == 0) return "text/css";
    if (strcmp(dot, ".js") == 0) return "application/javascript";
    if (strcmp(dot, ".json") == 0) return "application/json";
    if (strcmp(dot, ".ico") == 0) return "image/x-icon";
    if (strcmp(dot, ".png") == 0) return "image/png";
    if (strcmp(dot, ".svg") == 0) return "image/svg+xml";
    if (strcmp(dot, ".txt") == 0) return "text/plain";
    if (strcmp(dot, ".xml") == 0) return "application/xml";
    return "application/octet-stream";
}

#endif
