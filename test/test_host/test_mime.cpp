#include "test_host.h"

#include "http_static_files.h"

bool run_test_mime()
{
#if HTTP_ENABLE_STATIC_FILES
    CHECK_STR(http_static_mime_type("/x/index.html"), "text/html; charset=utf-8");
    CHECK_STR(http_static_mime_type("/x/a.css"), "text/css");
    CHECK_STR(http_static_mime_type("/x/a.js"), "application/javascript");
    CHECK_STR(http_static_mime_type("/x/a.json"), "application/json");
    CHECK_STR(http_static_mime_type("/x/a.ico"), "image/x-icon");
    CHECK_STR(http_static_mime_type("/x/a.png"), "image/png");
    CHECK_STR(http_static_mime_type("/x/a.svg"), "image/svg+xml");
    CHECK_STR(http_static_mime_type("/x/a.txt"), "text/plain");
    CHECK_STR(http_static_mime_type("/x/a.xml"), "application/xml");
    CHECK_STR(http_static_mime_type("/x/a.bin"), "application/octet-stream");
#endif
    return true;
}
