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

HttpErr HttpResponse::append(const char *s)
{
    return append_bytes(reinterpret_cast<const uint8_t *>(s), strlen(s));
}

HttpErr HttpResponse::append_bytes(const uint8_t *data, size_t len)
{
    if (buffer_ == 0 || data == 0 || data_len_ + len > buffer_len_) {
        return HttpErr::SEND_FAILED;
    }
    memcpy(buffer_ + data_len_, data, len);
    data_len_ += len;
    if (data_len_ < buffer_len_) {
        buffer_[data_len_] = 0;
    }
    return HttpErr::OK;
}

HttpErr HttpResponse::append_uint(size_t value)
{
    char tmp[16];
    snprintf(tmp, sizeof(tmp), "%lu", static_cast<unsigned long>(value));
    return append(tmp);
}

HttpErr HttpResponse::build_headers(size_t body_len, const char *content_type)
{
    HttpErr err;

    err = append("HTTP/1.1 ");
    if (err != HttpErr::OK) return err;
    err = append_uint(status_);
    if (err != HttpErr::OK) return err;
    err = append(" ");
    if (err != HttpErr::OK) return err;
    err = append(http_status_reason(status_));
    if (err != HttpErr::OK) return err;
    err = append("\r\n");
    if (err != HttpErr::OK) return err;

    if (find_header("Content-Type") < 0 && content_type != 0 && content_type[0] != '\0') {
        err = append("Content-Type: ");
        if (err != HttpErr::OK) return err;
        err = append(content_type);
        if (err != HttpErr::OK) return err;
        err = append("\r\n");
        if (err != HttpErr::OK) return err;
    }

    for (uint8_t i = 0; i < header_count_; i++) {
        err = append(header_names_[i]);
        if (err != HttpErr::OK) return err;
        err = append(": ");
        if (err != HttpErr::OK) return err;
        err = append(header_values_[i]);
        if (err != HttpErr::OK) return err;
        err = append("\r\n");
        if (err != HttpErr::OK) return err;
    }

    err = append("Content-Length: ");
    if (err != HttpErr::OK) return err;
    err = append_uint(body_len);
    if (err != HttpErr::OK) return err;
    err = append("\r\nConnection: close\r\n\r\n");
    return err;
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
    if (sent_) {
        return HttpErr::ALREADY_SENT;
    }
    if (data == 0 && len != 0) {
        return HttpErr::INVALID_ARG;
    }

    data_len_ = 0;
    HttpErr err = build_headers(len, content_type);
    if (err != HttpErr::OK) {
        return err;
    }
    err = append_bytes(data, len);
    if (err != HttpErr::OK) {
        return err;
    }
    sent_ = true;
    return HttpErr::OK;
}

HttpErr HttpResponse::send_json(const char *json)
{
    return send(reinterpret_cast<const uint8_t *>(json), json != 0 ? strlen(json) : 0, "application/json");
}

HttpErr HttpResponse::send_html(const char *html)
{
    return send(reinterpret_cast<const uint8_t *>(html), html != 0 ? strlen(html) : 0, "text/html; charset=utf-8");
}

HttpErr HttpResponse::end(uint16_t status)
{
    if (sent_) {
        return HttpErr::ALREADY_SENT;
    }
    status_ = status;
    data_len_ = 0;
    HttpErr err = build_headers(0, "");
    if (err != HttpErr::OK) {
        return err;
    }
    sent_ = true;
    return HttpErr::OK;
}

bool HttpResponse::sent() const { return sent_; }
const uint8_t *HttpResponse::data() const { return buffer_; }
size_t HttpResponse::data_len() const { return data_len_; }
uint16_t HttpResponse::status() const { return status_; }
