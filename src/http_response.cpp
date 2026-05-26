#include "http_response.h"

#include "http_common.h"

#include <stdio.h>
#include <string.h>

HttpResponse::HttpResponse()
{
    init(0, 0);
}

void HttpResponse::init(uint8_t *buffer, size_t buffer_len)
{
    buffer_ = buffer;
    buffer_len_ = buffer_len;
    data_len_ = 0;
    status_ = 200;
    sent_ = false;
    header_count_ = 0;
    content_type_[0] = '\0';
    body_kind_ = HttpBodyKind::NONE;
    body_len_ = 0;
    body_ram_ = 0;
    body_progmem_ = 0;
    body_reader_ = 0;
    body_reader_ctx_ = 0;
}

void HttpResponse::set_status(uint16_t status)
{
    if (!sent_) {
        status_ = status;
    }
}

int HttpResponse::find_header(const char *name) const
{
    for (uint8_t i = 0; i < header_count_; i++) {
        if (http_str_case_equal(header_names_[i], name)) {
            return i;
        }
    }
    return -1;
}

HttpErr HttpResponse::set_header(const char *name, const char *value)
{
    if (sent_) {
        return HttpErr::ALREADY_SENT;
    }
    if (name == 0 || value == 0 || name[0] == '\0') {
        return HttpErr::INVALID_ARG;
    }

    int idx = find_header(name);
    if (idx < 0) {
        if (header_count_ >= HTTP_RESP_HEADER_MAX) {
            return HttpErr::HEADER_FULL;
        }
        idx = header_count_++;
    }

    strncpy(header_names_[idx], name, HTTP_RESP_HEADER_NAME_MAX);
    header_names_[idx][HTTP_RESP_HEADER_NAME_MAX] = '\0';
    strncpy(header_values_[idx], value, HTTP_RESP_HEADER_VALUE_MAX);
    header_values_[idx][HTTP_RESP_HEADER_VALUE_MAX] = '\0';
    return HttpErr::OK;
}

HttpErr HttpResponse::set_content_type(const char *value)
{
    return set_header("Content-Type", value);
}

HttpErr HttpResponse::send(const char *text)
{
    if (text == 0) {
        return HttpErr::INVALID_ARG;
    }
    return send(reinterpret_cast<const uint8_t *>(text), strlen(text), "text/plain");
}

HttpErr HttpResponse::send(const uint8_t *data, size_t len, const char *content_type)
{
    return finalize_descriptor(HttpBodyKind::RAM, len, content_type, data, 0, 0, 0);
}

HttpErr HttpResponse::send_json(const char *json)
{
    return send(reinterpret_cast<const uint8_t *>(json), json != 0 ? strlen(json) : 0, "application/json");
}

HttpErr HttpResponse::send_html(const char *html)
{
    return send(reinterpret_cast<const uint8_t *>(html), html != 0 ? strlen(html) : 0, "text/html; charset=utf-8");
}

HttpErr HttpResponse::send_progmem(const uint8_t *data_progmem, size_t len, const char *content_type)
{
    return finalize_descriptor(HttpBodyKind::PROGMEM, len, content_type, 0, data_progmem, 0, 0);
}

HttpErr HttpResponse::send_html_progmem(const char *html_progmem)
{
    if (html_progmem == 0) {
        return HttpErr::INVALID_ARG;
    }
    return send_progmem(reinterpret_cast<const uint8_t *>(html_progmem),
                        strlen(html_progmem),
                        "text/html; charset=utf-8");
}

HttpErr HttpResponse::send_stream(HttpBodyReader reader,
                                  void *ctx,
                                  size_t content_length,
                                  const char *content_type)
{
    return finalize_descriptor(HttpBodyKind::CALLBACK,
                               content_length,
                               content_type,
                               0,
                               0,
                               reader,
                               ctx);
}

HttpErr HttpResponse::end(uint16_t status)
{
    if (sent_) {
        return HttpErr::ALREADY_SENT;
    }
    status_ = status;
    return finalize_descriptor(HttpBodyKind::NONE, 0, "", 0, 0, 0, 0);
}

bool HttpResponse::sent() const { return sent_; }
const uint8_t *HttpResponse::data() const { return buffer_; }
size_t HttpResponse::data_len() const { return data_len_; }
uint16_t HttpResponse::status() const { return status_; }
HttpBodyKind HttpResponse::body_kind() const { return body_kind_; }
size_t HttpResponse::body_length() const { return body_len_; }
const uint8_t *HttpResponse::body_ram() const { return body_ram_; }
const uint8_t *HttpResponse::body_progmem() const { return body_progmem_; }
HttpBodyReader HttpResponse::body_reader() const { return body_reader_; }
void *HttpResponse::body_reader_ctx() const { return body_reader_ctx_; }
const char *HttpResponse::content_type() const { return content_type_; }

HttpErr HttpResponse::finalize_descriptor(HttpBodyKind kind,
                                          size_t body_len,
                                          const char *content_type,
                                          const uint8_t *ram,
                                          const uint8_t *progmem,
                                          HttpBodyReader reader,
                                          void *reader_ctx)
{
    if (sent_) {
        return HttpErr::ALREADY_SENT;
    }
    if ((kind == HttpBodyKind::RAM && ram == 0 && body_len != 0) ||
        (kind == HttpBodyKind::PROGMEM && progmem == 0 && body_len != 0) ||
        (kind == HttpBodyKind::CALLBACK && reader == 0 && body_len != 0)) {
        return HttpErr::INVALID_ARG;
    }

    if (content_type == 0) {
        content_type = "";
    }
    strncpy(content_type_, content_type, HTTP_RESP_HEADER_VALUE_MAX);
    content_type_[HTTP_RESP_HEADER_VALUE_MAX] = '\0';

    body_kind_ = kind;
    body_len_ = body_len;
    body_ram_ = ram;
    body_progmem_ = progmem;
    body_reader_ = reader;
    body_reader_ctx_ = reader_ctx;

    sent_ = true;
    data_len_ = 0;
    return HttpErr::OK;
}

uint8_t HttpResponse::header_count() const { return header_count_; }

const char *HttpResponse::header_name(uint8_t index) const
{
    if (index >= header_count_) {
        return 0;
    }
    return header_names_[index];
}

const char *HttpResponse::header_value(uint8_t index) const
{
    if (index >= header_count_) {
        return 0;
    }
    return header_values_[index];
}
