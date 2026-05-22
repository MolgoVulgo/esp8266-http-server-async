#include "test_host.h"

#include "http_config.h"
#include "http_fs_backend.h"
#include "http_request.h"
#include "http_response.h"
#include "http_server.h"
#include "http_types.h"

bool run_test_build()
{
    CHECK_TRUE(HTTP_MAX_ROUTES == 16);
    CHECK_TRUE(HTTP_MAX_CLIENTS == 3);
    CHECK_TRUE(HTTP_ENABLE_STATIC_FILES == 1);
    HttpServer server(80);
    (void)server;
    return true;
}
