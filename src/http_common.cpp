#include "http_types.h"

#include <ctype.h>
#include <string.h>

const char *http_method_name(HttpMethod method)
{
    switch (method) {
    case HttpMethod::GET: return "GET";
    case HttpMethod::POST: return "POST";
    case HttpMethod::PUT: return "PUT";
    case HttpMethod::DELETE: return "DELETE";
    case HttpMethod::OPTIONS: return "OPTIONS";
    default: return "UNKNOWN";
    }
}

HttpMethod http_method_from_cstr(const char *method)
{
    if (method == 0) {
        return HttpMethod::UNKNOWN;
    }
    if (strcmp(method, "GET") == 0) {
        return HttpMethod::GET;
    }
    if (strcmp(method, "POST") == 0) {
        return HttpMethod::POST;
    }
    if (strcmp(method, "PUT") == 0) {
        return HttpMethod::PUT;
    }
    if (strcmp(method, "DELETE") == 0) {
        return HttpMethod::DELETE;
    }
    if (strcmp(method, "OPTIONS") == 0) {
        return HttpMethod::OPTIONS;
    }
    return HttpMethod::UNKNOWN;
}

const char *http_status_reason(uint16_t status)
{
    switch (status) {
    case 200: return "OK";
    case 204: return "No Content";
    case 400: return "Bad Request";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 413: return "Payload Too Large";
    case 500: return "Internal Server Error";
    default: return "OK";
    }
}

bool http_str_case_equal(const char *a, const char *b)
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

bool http_str_starts_with_case(const char *s, const char *prefix)
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
