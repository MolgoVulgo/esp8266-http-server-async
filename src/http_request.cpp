#include "http_request.h"

#include <ctype.h>
#include <string.h>
#include "http_url_decode.h"

static bool case_equal(const char *a, const char *b)
{
    if (a == 0 || b == 0) {
        return false;
    }
    while (*a != '\0' && *b != '\0') {
        if (tolower(static_cast<unsigned char>(*a)) !=
            tolower(static_cast<unsigned char>(*b))) {
            return false;
        }
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

static bool starts_with_case(const char *s, const char *prefix)
{
    if (s == 0 || prefix == 0) {
        return false;
    }
    while (*prefix != '\0') {
        if (tolower(static_cast<unsigned char>(*s)) !=
            tolower(static_cast<unsigned char>(*prefix))) {
            return false;
        }
        s++;
        prefix++;
    }
    return true;
}

HttpRequest::HttpRequest()
{
    init(HttpMethod::UNKNOWN, "", "", 0, 0, 0, 0, 0, 0);
}

void HttpRequest::init(HttpMethod method,
                       const char *path,
                       const char *query_raw,
                       const HttpParsedHeaderView *headers,
                       uint8_t header_count,
                       const uint8_t *body,
                       size_t body_len,
                       void *user_ctx,
                       const HttpRouteMatch *route_match)
{
    method_ = method;
    path_ = path != 0 ? path : "";
    query_raw_ = query_raw != 0 ? query_raw : "";
    headers_ = headers;
    header_count_ = header_count;
    body_ = body;
    body_len_ = body_len;
    user_ctx_ = user_ctx;
    route_match_ = route_match;
}

HttpMethod HttpRequest::method() const { return method_; }
const char *HttpRequest::path() const { return path_; }
const char *HttpRequest::query_raw() const { return query_raw_; }
const uint8_t *HttpRequest::body() const { return body_; }
size_t HttpRequest::body_len() const { return body_len_; }
void *HttpRequest::user_ctx() const { return user_ctx_; }

const char *HttpRequest::header(const char *name) const
{
    if (headers_ == 0 || name == 0) {
        return 0;
    }
    for (uint8_t i = 0; i < header_count_; i++) {
        if (case_equal(headers_[i].name, name)) {
            return headers_[i].value;
        }
    }
    return 0;
}

bool HttpRequest::find_urlencoded_param(const char *src,
                                        size_t src_len,
                                        const char *name,
                                        char *out,
                                        size_t out_len) const
{
    size_t name_len;
    size_t pos = 0;

    if (src == 0 || name == 0 || out == 0 || out_len == 0) {
        return false;
    }

    name_len = strlen(name);
    while (pos <= src_len) {
        size_t key_start = pos;
        size_t key_len = 0;
        size_t val_start = 0;
        size_t val_len = 0;

        while (pos < src_len && src[pos] != '=' && src[pos] != '&' && src[pos] != '\0') {
            pos++;
        }
        key_len = pos - key_start;

        if (pos < src_len && src[pos] == '=') {
            pos++;
            val_start = pos;
            while (pos < src_len && src[pos] != '&' && src[pos] != '\0') {
                pos++;
            }
            val_len = pos - val_start;
        } else {
            val_start = pos;
            val_len = 0;
        }

        if (key_len == name_len && strncmp(&src[key_start], name, name_len) == 0) {
            return http_url_decode(&src[val_start], val_len, out, out_len);
        }

        if (pos >= src_len || src[pos] == '\0') {
            break;
        }
        pos++;
    }

    return false;
}

bool HttpRequest::param(const char *name, char *out, size_t out_len) const
{
    return find_urlencoded_param(query_raw_, strlen(query_raw_), name, out, out_len);
}

bool HttpRequest::form_param(const char *name, char *out, size_t out_len) const
{
    const char *content_type = header("Content-Type");
    if (!starts_with_case(content_type, "application/x-www-form-urlencoded") ||
        body_ == 0 || body_len_ == 0) {
        return false;
    }
    return find_urlencoded_param(reinterpret_cast<const char *>(body_), body_len_, name, out, out_len);
}

bool HttpRequest::route_param(const char *name, char *out, size_t out_len) const
{
    (void)name;
    (void)out;
    (void)out_len;
    (void)route_match_;
    return false;
}
