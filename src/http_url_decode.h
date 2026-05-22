#ifndef HTTP_URL_DECODE_INTERNAL_H
#define HTTP_URL_DECODE_INTERNAL_H

#include <stddef.h>

bool http_url_decode(const char *src, size_t src_len, char *out, size_t out_len);

#endif
