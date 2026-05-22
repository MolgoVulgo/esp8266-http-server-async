#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_common.h"

#ifndef WIFI_SSID
#define WIFI_SSID "your-ssid"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "your-password"
#endif

extern void http_test_start_server_once(void);

void http_test_log_status(const char *label, int value)
{
    os_printf("http_test: %s=%d\n", label, value);
}

uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();

    switch (size_map) {
    case FLASH_SIZE_4M_MAP_256_256:
        return 128 - 5;
    case FLASH_SIZE_8M_MAP_512_512:
        return 256 - 5;
    case FLASH_SIZE_16M_MAP_512_512:
    case FLASH_SIZE_16M_MAP_1024_1024:
        return 512 - 5;
    case FLASH_SIZE_32M_MAP_512_512:
    case FLASH_SIZE_32M_MAP_1024_1024:
        return 1024 - 5;
    case FLASH_SIZE_64M_MAP_1024_1024:
        return 2048 - 5;
    case FLASH_SIZE_128M_MAP_1024_1024:
        return 4096 - 5;
    default:
        return 0;
    }
}

static void wifi_event_handler_cb(System_Event_t *event)
{
    if (event == NULL) {
        return;
    }

    switch (event->event_id) {
    case EVENT_STAMODE_GOT_IP:
        os_printf("http_test: wifi got ip\n");
        http_test_start_server_once();
        break;

    case EVENT_STAMODE_DISCONNECTED:
        os_printf("http_test: wifi disconnected\n");
        break;

    default:
        break;
    }
}

void user_init(void)
{
    os_printf("http_test: SDK version %s heap=%u\n",
              system_get_sdk_version(),
              system_get_free_heap_size());

    wifi_set_opmode(STATION_MODE);
    wifi_set_event_handler_cb(wifi_event_handler_cb);

    struct station_config config;
    memset(&config, 0, sizeof(config));
    snprintf((char *)config.ssid, sizeof(config.ssid), "%s", WIFI_SSID);
    snprintf((char *)config.password, sizeof(config.password), "%s", WIFI_PASSWORD);

    wifi_station_set_config(&config);
    wifi_station_connect();
}
