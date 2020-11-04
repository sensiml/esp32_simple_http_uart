#ifndef __HTTPD_TASK_H__
#define __HTTPD_TASK_H__

#define DEFAULT_WIFI_MODE WIFI_MODE_AP
#define DEFAULT_WIFI_STA_SSID CONFIG_EXAMPLE_WIFI_SSID
#define DEFAULT_WIFI_STA_PASS CONFIG_EXAMPLE_WIFI_PASSWORD
#define LISTEN_PORT     80u
#define MAX_CONNECTIONS 2u

void httpd_task_init();
bool httpd_task_is_stream_open();
#endif
