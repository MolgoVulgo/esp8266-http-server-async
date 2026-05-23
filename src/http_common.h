#ifndef HTTP_COMMON_H
#define HTTP_COMMON_H

#include <stdint.h>
#include "http_types.h"

const char *http_method_name(HttpMethod method);
HttpMethod http_method_from_cstr(const char *method);
const char *http_status_reason(uint16_t status);

bool http_str_case_equal(const char *a, const char *b);
bool http_str_starts_with_case(const char *s, const char *prefix);

#endif
