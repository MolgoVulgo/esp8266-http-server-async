#include "http_server.h"

static HttpServer server(80);

static void handle_config(HttpRequest &req, HttpResponse &res)
{
    char name[32];
    if (req.form_param("name", name, sizeof(name))) {
        res.end(204);
        return;
    }
    res.set_status(400);
    res.send("missing name");
}

extern "C" void app_main(void)
{
    server.on(HttpMethod::POST, "/api/config", handle_config);
    server.begin();
}
