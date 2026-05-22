#include "http_parser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool str_case_equal(const char *a, const char *b)
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

HttpParser::HttpParser()
{
    reset();
}

void HttpParser::reset()
{
    state_ = HttpParserState::READ_LINE;
    line_len_ = 0;
    header_storage_len_ = 0;
    header_count_ = 0;
    path_[0] = '\0';
    query_[0] = '\0';
    version_[0] = '\0';
    body_len_ = 0;
    content_length_ = 0;
    content_length_seen_ = false;
    method_ = HttpMethod::UNKNOWN;
    error_ = false;
}

HttpParserResult HttpParser::feed(const uint8_t *data, size_t len)
{
    if (data == 0 && len != 0) {
        set_error();
        return HttpParserResult::ERROR;
    }

    for (size_t i = 0; i < len; i++) {
        HttpParserResult r = consume_char(static_cast<char>(data[i]));
        if (r != HttpParserResult::NEED_MORE) {
            return r;
        }
    }

    if (state_ == HttpParserState::DONE) {
        return HttpParserResult::DONE;
    }
    if (state_ == HttpParserState::ERROR) {
        return HttpParserResult::ERROR;
    }
    return HttpParserResult::NEED_MORE;
}

HttpParserResult HttpParser::consume_char(char c)
{
    if (state_ == HttpParserState::READ_LINE ||
        state_ == HttpParserState::READ_HEADERS) {
        if (c == '\r') {
            return HttpParserResult::NEED_MORE;
        }
        if (c == '\n') {
            line_[line_len_] = '\0';
            bool ok = false;
            if (state_ == HttpParserState::READ_LINE) {
                ok = parse_request_line();
                state_ = HttpParserState::READ_HEADERS;
            } else if (line_len_ == 0) {
                ok = finish_headers();
            } else {
                ok = parse_header_line();
            }
            line_len_ = 0;
            if (!ok) {
                if (content_length_ > HTTP_BODY_MAX_SIZE) {
                    set_error();
                    return HttpParserResult::BODY_TOO_LARGE;
                }
                set_error();
                return HttpParserResult::ERROR;
            }
            if (state_ == HttpParserState::DONE) {
                return HttpParserResult::DONE;
            }
            return HttpParserResult::NEED_MORE;
        }
        if (line_len_ >= HTTP_LINE_MAX) {
            set_error();
            return HttpParserResult::ERROR;
        }
        line_[line_len_++] = c;
        return HttpParserResult::NEED_MORE;
    }

    if (state_ == HttpParserState::READ_BODY) {
        if (body_len_ >= HTTP_BODY_MAX_SIZE) {
            set_error();
            return HttpParserResult::BODY_TOO_LARGE;
        }
        body_[body_len_++] = static_cast<uint8_t>(c);
        if (body_len_ == content_length_) {
            body_[body_len_] = 0;
            state_ = HttpParserState::DONE;
            return HttpParserResult::DONE;
        }
        return HttpParserResult::NEED_MORE;
    }

    return state_ == HttpParserState::DONE ? HttpParserResult::DONE : HttpParserResult::ERROR;
}

bool HttpParser::parse_request_line()
{
    char method[12];
    char target[HTTP_LINE_MAX + 1];
    char version[16];
    method[0] = '\0';
    target[0] = '\0';
    version[0] = '\0';

    if (sscanf(line_, "%11s %128s %15s", method, target, version) != 3) {
        return false;
    }
    if (strncmp(version, "HTTP/", 5) != 0) {
        return false;
    }

    method_ = http_method_from_cstr(method);
    strncpy(version_, version, sizeof(version_) - 1);
    version_[sizeof(version_) - 1] = '\0';

    char *query = strchr(target, '?');
    if (query != 0) {
        *query = '\0';
        query++;
        if (strlen(query) > HTTP_QUERY_MAX_SIZE) {
            return false;
        }
        strncpy(query_, query, sizeof(query_) - 1);
        query_[sizeof(query_) - 1] = '\0';
    } else {
        query_[0] = '\0';
    }

    if (target[0] != '/' || strlen(target) > HTTP_LINE_MAX) {
        return false;
    }
    strncpy(path_, target, sizeof(path_) - 1);
    path_[sizeof(path_) - 1] = '\0';
    return true;
}

bool HttpParser::append_header_storage(const char *line, size_t len, char **out)
{
    if (header_storage_len_ + len + 1 > HTTP_HEADER_MAX_SIZE) {
        return false;
    }
    char *dst = &header_storage_[header_storage_len_];
    memcpy(dst, line, len);
    dst[len] = '\0';
    header_storage_len_ += len + 1;
    *out = dst;
    return true;
}

bool HttpParser::parse_header_line()
{
    if (header_count_ >= HTTP_HEADER_MAX_COUNT) {
        return false;
    }

    char *colon = strchr(line_, ':');
    if (colon == 0 || colon == line_) {
        return false;
    }

    *colon = '\0';
    char *name = line_;
    char *value = colon + 1;
    while (*value == ' ' || *value == '\t') {
        value++;
    }

    if (strlen(name) > HTTP_HEADER_NAME_MAX) {
        return false;
    }

    char *stored_name = 0;
    char *stored_value = 0;
    if (!append_header_storage(name, strlen(name), &stored_name) ||
        !append_header_storage(value, strlen(value), &stored_value)) {
        return false;
    }

    headers_[header_count_].name = stored_name;
    headers_[header_count_].value = stored_value;
    header_count_++;

    if (str_case_equal(stored_name, "Content-Length")) {
        char *end = 0;
        unsigned long v = strtoul(stored_value, &end, 10);
        if (end == stored_value || *end != '\0') {
            return false;
        }
        content_length_ = static_cast<size_t>(v);
        content_length_seen_ = true;
        if (content_length_ > HTTP_BODY_MAX_SIZE) {
            return false;
        }
    }

    return true;
}

bool HttpParser::finish_headers()
{
    if (content_length_seen_ && content_length_ > 0) {
        state_ = HttpParserState::READ_BODY;
    } else {
        body_len_ = 0;
        body_[0] = 0;
        state_ = HttpParserState::DONE;
    }
    return true;
}

void HttpParser::set_error()
{
    error_ = true;
    state_ = HttpParserState::ERROR;
}

HttpParserState HttpParser::state() const { return state_; }
HttpMethod HttpParser::method() const { return method_; }
const char *HttpParser::path() const { return path_; }
const char *HttpParser::query_raw() const { return query_; }
const uint8_t *HttpParser::body() const { return body_; }
size_t HttpParser::body_len() const { return body_len_; }
uint8_t HttpParser::header_count() const { return header_count_; }
const HttpParsedHeaderView *HttpParser::headers() const { return headers_; }
bool HttpParser::has_error() const { return error_; }

const char *HttpParser::header(const char *name) const
{
    for (uint8_t i = 0; i < header_count_; i++) {
        if (str_case_equal(headers_[i].name, name)) {
            return headers_[i].value;
        }
    }
    return 0;
}
