#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <stddef.h>
#include <stdint.h>
#include "http_types.h"

typedef size_t (*HttpBodyReader)(void *ctx, size_t offset, uint8_t *dst, size_t max_len);

enum class HttpBodyKind : uint8_t {
    NONE = 0,
    RAM,
    PROGMEM,
    CALLBACK
};

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
    HttpErr send_progmem(const uint8_t *data_progmem, size_t len, const char *content_type);
    HttpErr send_html_progmem(const char *html_progmem);
    HttpErr send_stream(HttpBodyReader reader,
                        void *ctx,
                        size_t content_length,
                        const char *content_type);
    HttpErr end(uint16_t status = 204);

    bool sent() const;
    const uint8_t *data() const;
    size_t data_len() const;
    uint16_t status() const;
    HttpBodyKind body_kind() const;
    size_t body_length() const;
    const uint8_t *body_ram() const;
    const uint8_t *body_progmem() const;
    HttpBodyReader body_reader() const;
    void *body_reader_ctx() const;
    const char *content_type() const;
    uint8_t header_count() const;
    const char *header_name(uint8_t index) const;
    const char *header_value(uint8_t index) const;

private:
    HttpErr finalize_descriptor(HttpBodyKind kind,
                                size_t body_len,
                                const char *content_type,
                                const uint8_t *ram,
                                const uint8_t *progmem,
                                HttpBodyReader reader,
                                void *reader_ctx);
    int find_header(const char *name) const;

    uint8_t *buffer_;
    size_t buffer_len_;
    size_t data_len_;
    uint16_t status_;
    bool sent_;
    uint8_t header_count_;
    char header_names_[HTTP_RESP_HEADER_MAX][HTTP_RESP_HEADER_NAME_MAX + 1];
    char header_values_[HTTP_RESP_HEADER_MAX][HTTP_RESP_HEADER_VALUE_MAX + 1];
    char content_type_[HTTP_RESP_HEADER_VALUE_MAX + 1];

    HttpBodyKind body_kind_;
    size_t body_len_;
    const uint8_t *body_ram_;
    const uint8_t *body_progmem_;
    HttpBodyReader body_reader_;
    void *body_reader_ctx_;
};

#endif
