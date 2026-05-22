#include "http_types.h"

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
