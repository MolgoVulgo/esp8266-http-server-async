#include "http_url_decode.h"

static int hex_value(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

bool http_url_decode(const char *src, size_t src_len, char *out, size_t out_len)
{
    size_t si = 0;
    size_t oi = 0;

    if (src == 0 || out == 0 || out_len == 0) {
        return false;
    }

    while (si < src_len && src[si] != '\0') {
        char c = src[si];

        if (oi + 1 >= out_len) {
            return false;
        }

        if (c == '+') {
            out[oi++] = ' ';
            si++;
        } else if (c == '%') {
            if (si + 2 >= src_len || src[si + 1] == '\0' || src[si + 2] == '\0') {
                return false;
            }
            int hi = hex_value(src[si + 1]);
            int lo = hex_value(src[si + 2]);
            if (hi < 0 || lo < 0) {
                return false;
            }
            out[oi++] = static_cast<char>((hi << 4) | lo);
            si += 3;
        } else {
            out[oi++] = c;
            si++;
        }
    }

    out[oi] = '\0';
    return true;
}
