#ifndef HTTP_TYPES_H
#define HTTP_TYPES_H

#include <stdint.h>
#include "http_config.h"

class HttpRequest;
class HttpResponse;

enum class HttpMethod : uint8_t {
    GET     = 0,
    POST    = 1,
    PUT     = 2,
    DELETE  = 3,
    OPTIONS = 4,
    UNKNOWN = 0xFF
};

enum class HttpErr : int8_t {
    OK              =  0,
    INVALID_ARG     = -1,
    NO_SLOT         = -2,
    ROUTE_FULL      = -3,
    HEADER_FULL     = -4,
    SEND_FAILED     = -5,
    PARSE_ERROR     = -6,
    TIMEOUT         = -7,
    FS_ERROR        = -8,
    ALREADY_SENT    = -9,
    NOT_STARTED     = -10,
};

struct HttpRouteParam {
    char name[HTTP_PARAM_NAME_MAX + 1];
    char value[HTTP_PARAM_VALUE_MAX + 1];
};

struct HttpRouteMatch {
    uint8_t count;
    HttpRouteParam params[HTTP_ROUTE_PARAM_MAX];
};

typedef void (*HttpHandler)(HttpRequest &req, HttpResponse &res);

typedef bool (*HttpRouteMatcher)(
    const char     *pattern,
    const char     *path,
    HttpRouteMatch *out_match
);

bool http_route_match_exact(const char *pattern,
                            const char *path,
                            HttpRouteMatch *out_match);

const char *http_method_name(HttpMethod method);
HttpMethod http_method_from_cstr(const char *method);
const char *http_status_reason(uint16_t status);

#endif
