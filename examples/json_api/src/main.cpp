#include "http_server.h"

static HttpServer server(80);

static void handle_status(HttpRequest &, HttpResponse &res)
{
    res.send_json("{\"status\":\"ok\"}");
}

extern "C" void app_main(void)
{
    server.on(HttpMethod::GET, "/api/status", handle_status);
    server.begin();
}
