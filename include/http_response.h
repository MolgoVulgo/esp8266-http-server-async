#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <stddef.h>
#include <stdint.h>
#include "http_types.h"

class HttpResponse {
public:
    HttpResponse();

    void init(uint8_t *buffer, size_t buffer_len);

    void set_status(uint16_t status);
    HttpErr set_header(const char *name, const char *value);
    HttpErr set_content_type(const char *value);

    HttpErr send(const char *text);
    HttpErr send(const uint8_t *data, size_t len, const char *content_type = "application/octet-stream");
    HttpErr send_json(const char *json);
    HttpErr send_html(const char *html);
    HttpErr end(uint16_t status = 204);

    bool sent() const;
    const uint8_t *data() const;
    size_t data_len() const;
    uint16_t status() const;

private:
    HttpErr append(const char *s);
    HttpErr append_bytes(const uint8_t *data, size_t len);
    HttpErr append_uint(size_t value);
    HttpErr build_headers(size_t body_len, const char *content_type);
    int find_header(const char *name) const;

    uint8_t *buffer_;
    size_t buffer_len_;
    size_t data_len_;
    uint16_t status_;
    bool sent_;
    uint8_t header_count_;
    char header_names_[HTTP_RESP_HEADER_MAX][HTTP_RESP_HEADER_NAME_MAX + 1];
    char header_values_[HTTP_RESP_HEADER_MAX][HTTP_RESP_HEADER_VALUE_MAX + 1];
};

#endif
