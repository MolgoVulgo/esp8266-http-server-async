#ifndef TEST_HOST_H
#define TEST_HOST_H

#include <stdio.h>
#include <string.h>

#define CHECK_TRUE(expr) do { \
    if (!(expr)) { \
        printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        return false; \
    } \
} while (0)

#define CHECK_EQ_INT(a, b) CHECK_TRUE((int)(a) == (int)(b))
#define CHECK_STR(a, b) CHECK_TRUE(strcmp((a), (b)) == 0)
#define CHECK_CONTAINS(haystack, needle) CHECK_TRUE(strstr((haystack), (needle)) != 0)

bool run_test_build();
bool run_test_url_decode();
bool run_test_parser();
bool run_test_request();
bool run_test_response();
bool run_test_router();
bool run_test_static_path();
bool run_test_mime();
bool run_test_engine();

#endif
