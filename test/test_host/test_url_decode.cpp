#include "test_host.h"

#include "http_url_decode.h"

bool run_test_url_decode()
{
    char out[32];
    CHECK_TRUE(http_url_decode("abc", 3, out, sizeof(out)));
    CHECK_STR(out, "abc");
    CHECK_TRUE(http_url_decode("a%20b", 5, out, sizeof(out)));
    CHECK_STR(out, "a b");
    CHECK_TRUE(http_url_decode("a+b", 3, out, sizeof(out)));
    CHECK_STR(out, "a b");
    CHECK_TRUE(!http_url_decode("a%XX", 4, out, sizeof(out)));
    CHECK_TRUE(!http_url_decode("abcdef", 6, out, 4));
    CHECK_TRUE(http_url_decode("x%3Dy", 5, out, sizeof(out)));
    CHECK_STR(out, "x=y");
    return true;
}
