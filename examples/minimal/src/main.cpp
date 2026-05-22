#include "http_server.h"

static HttpServer server(80);

static void handle_root(HttpRequest &, HttpResponse &res)
{
    res.send_html("<!doctype html><html><body><h1>ok</h1></body></html>");
}

extern "C" void app_main(void)
{
    server.on(HttpMethod::GET, "/", handle_root);
    server.begin();
}
