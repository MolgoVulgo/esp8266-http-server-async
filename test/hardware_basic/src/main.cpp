#include "http_server.h"

#ifndef HTTP_TEST_PORT
#define HTTP_TEST_PORT 80
#endif

static HttpServer s_server((uint16_t)HTTP_TEST_PORT);
static bool s_server_started;

extern "C" void http_test_log_status(const char *label, int value);

static void handle_root(HttpRequest &, HttpResponse &res)
{
    res.send_html("<!doctype html><html><body><h1>esp8266-http ok</h1></body></html>");
}

static void handle_status(HttpRequest &, HttpResponse &res)
{
    res.send_json("{\"status\":\"ok\",\"target\":\"esp8266\"}");
}

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

extern "C" void http_test_start_server_once(void)
{
    if (s_server_started) {
        return;
    }

    HttpErr err;
    err = s_server.on(HttpMethod::GET, "/", handle_root);
    if (err != HttpErr::OK) {
        http_test_log_status("route_root_err", (int)err);
        return;
    }

    err = s_server.on(HttpMethod::GET, "/api/status", handle_status);
    if (err != HttpErr::OK) {
        http_test_log_status("route_status_err", (int)err);
        return;
    }

    err = s_server.on(HttpMethod::POST, "/api/config", handle_config);
    if (err != HttpErr::OK) {
        http_test_log_status("route_config_err", (int)err);
        return;
    }

    err = s_server.begin();
    if (err != HttpErr::OK) {
        http_test_log_status("begin_err", (int)err);
        return;
    }

    s_server_started = true;
    http_test_log_status("listening", HTTP_TEST_PORT);
}
