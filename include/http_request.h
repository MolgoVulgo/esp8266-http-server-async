#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <stddef.h>
#include <stdint.h>
#include "http_types.h"

struct HttpParsedHeaderView {
    const char *name;
    const char *value;
};

class HttpRequest {
public:
    HttpRequest();

    HttpMethod method() const;
    const char *path() const;
    const char *query_raw() const;
    const uint8_t *body() const;
    size_t body_len() const;
    void *user_ctx() const;

    const char *header(const char *name) const;
    bool param(const char *name, char *out, size_t out_len) const;
    bool form_param(const char *name, char *out, size_t out_len) const;
    bool route_param(const char *name, char *out, size_t out_len) const;

    void init(HttpMethod method,
              const char *path,
              const char *query_raw,
              const HttpParsedHeaderView *headers,
              uint8_t header_count,
              const uint8_t *body,
              size_t body_len,
              void *user_ctx,
              const HttpRouteMatch *route_match);

private:
    bool find_urlencoded_param(const char *src,
                               size_t src_len,
                               const char *name,
                               char *out,
                               size_t out_len) const;

    HttpMethod method_;
    const char *path_;
    const char *query_raw_;
    const HttpParsedHeaderView *headers_;
    uint8_t header_count_;
    const uint8_t *body_;
    size_t body_len_;
    void *user_ctx_;
    const HttpRouteMatch *route_match_;
};

#endif
