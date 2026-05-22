#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <stddef.h>
#include <stdint.h>
#include "http_config.h"
#include "http_request.h"
#include "http_types.h"

enum class HttpParserState : uint8_t {
    READ_LINE,
    READ_HEADERS,
    READ_BODY,
    DONE,
    ERROR
};

enum class HttpParserResult : uint8_t {
    NEED_MORE,
    DONE,
    ERROR,
    BODY_TOO_LARGE
};

class HttpParser {
public:
    HttpParser();

    void reset();
    HttpParserResult feed(const uint8_t *data, size_t len);

    HttpParserState state() const;
    HttpMethod method() const;
    const char *path() const;
    const char *query_raw() const;
    const uint8_t *body() const;
    size_t body_len() const;
    uint8_t header_count() const;
    const HttpParsedHeaderView *headers() const;
    const char *header(const char *name) const;
    bool has_error() const;

private:
    HttpParserResult consume_char(char c);
    bool parse_request_line();
    bool parse_header_line();
    bool finish_headers();
    bool append_header_storage(const char *line, size_t len, char **out);
    void set_error();

    HttpParserState state_;
    char line_[HTTP_LINE_MAX + 1];
    size_t line_len_;
    char header_storage_[HTTP_HEADER_MAX_SIZE + 1];
    size_t header_storage_len_;
    HttpParsedHeaderView headers_[HTTP_HEADER_MAX_COUNT];
    uint8_t header_count_;
    char path_[HTTP_LINE_MAX + 1];
    char query_[HTTP_QUERY_MAX_SIZE + 1];
    char version_[16];
    uint8_t body_[HTTP_BODY_MAX_SIZE + 1];
    size_t body_len_;
    size_t content_length_;
    bool content_length_seen_;
    HttpMethod method_;
    bool error_;
};

#endif
