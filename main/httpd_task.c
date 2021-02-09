
#include "libesphttpd/cgiwifi.h"
#include "libesphttpd/cgiflash.h"
#include "libesphttpd/cgiwebsocket.h"
#include "libesphttpd/httpd-freertos.h"
#include "libesphttpd/httpd.h"
#include "libesphttpd/route.h"
#include "esp_wifi.h"
#include "driver/uart.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "cJSON.h"
#include "httpd_task.h"
#include "uart_task.h"

#define TAG "httpd_task"

char                               my_hostname[16] = "esphttpstream";
static CgiStatus ICACHE_FLASH_ATTR config_get_handler(HttpdConnData* connData);
static CgiStatus ICACHE_FLASH_ATTR stream_get_handler(HttpdConnData* connData);
static CgiStatus ICACHE_FLASH_ATTR results_get_handler(HttpdConnData* connData);
static char                        connectionMemory[sizeof(RtosConnType) * MAX_CONNECTIONS];
static HttpdFreertosInstance       httpdFreertosInstance;

HttpdBuiltInUrl builtInUrls[] = {
    ROUTE_CGI("/config", config_get_handler),
    ROUTE_CGI("/stream", stream_get_handler),
    ROUTE_CGI("/results", results_get_handler),
    ROUTE_END(),
};


static CgiStatus ICACHE_FLASH_ATTR config_get_handler(HttpdConnData* connData)
{
    if (connData->isConnectionClosed)
    {
        // Connection aborted. Clean up.
        return HTTPD_CGI_DONE;
    }
    cJSON* config_json = get_config_json();
    if (config_json != NULL)
    {
        httpdStartResponse(connData, 200);
        httpdHeader(connData, "Content-Type", "text/json");
        httpdEndHeaders(connData);
        const char* resp_str = (const char*) cJSON_Print(config_json);
        httpdSend(connData, resp_str, strlen(resp_str));
    }
    else
    {
        httpdStartResponse(connData, 200);
        httpdHeader(connData, "Content-Type", "text/plain");
        httpdEndHeaders(connData);
        const char* resp_str
            = "Config not yet received from device, or device is in recognition mode.";
        httpdSend(connData, resp_str, strlen(resp_str));
    }

    return HTTPD_CGI_DONE;
}

static CgiStatus ICACHE_FLASH_ATTR results_get_handler(HttpdConnData* connData)
{
    uint8_t* pData   = connData->cgiData;
    int      rxBytes = 0;

    if (connData->isConnectionClosed)
    {
        if (pData != NULL)
        {
            free(pData);
        }
        ESP_LOGI(TAG, "Result Stream Closed");
        // Connection aborted. Clean up.
        return HTTPD_CGI_DONE;
    }

    else
    {
        int queue_sz = get_result_queue_size();
        if (pData == NULL)
        {
            pData = (uint8_t*) malloc(RX_BUF_SIZE + 1);

            if (pData == NULL)
            {
                ESP_LOGE(TAG, "Cant malloc for result data");
                return HTTPD_CGI_DONE;
            }

            connData->cgiData = pData;
            // Set initial pointer to start of string
            // We need to send the headers before sending any data. Do that now.
            httpdStartResponse(connData, 200);
            httpdHeader(connData, "Content-Type", "application/octet-stream");
            httpdEndHeaders(connData);
            uart_flush_input(DEVICE_DATA_UART_NUM);
        }
        else
        {
            rxBytes = uart_read_bytes(
                DEVICE_DATA_UART_NUM, pData, queue_sz / 2, 100 / portTICK_RATE_MS);
            if (rxBytes > 0)
            {
                httpdSend(connData, (const char*) pData, rxBytes);
            }
            else
            {
                ESP_LOGW(TAG, "No bytes RX");
                const char* resp_str = "{}";
                httpdSend(connData, resp_str, strlen(resp_str));
            }
        }

        return HTTPD_CGI_MORE;
    }


    return HTTPD_CGI_DONE;
}

static CgiStatus stream_get_handler(HttpdConnData* connData)
{
    uint8_t* pSensorData = connData->cgiData;
    // If the browser unexpectedly closes the connection, the CGI will be called
    // after isConnectionClosed is set to true. We can use this to clean up any data. It's pretty
    // relevant here because otherwise we may leak memory when the browser aborts the connection.
    if (connData->isConnectionClosed)
    {
        // Connection aborted. Clean up.
        if (pSensorData != NULL)
        {
            // free(pSensorData->data);
            free(pSensorData);
        }
        ESP_LOGI(TAG, "Stream closed");
        return HTTPD_CGI_DONE;
    }

    else
    {
        int queue_sz = get_data_queue_size();
        if (pSensorData == NULL)
        {
            // This is the first call to the CGI for this webbrowser request.
            // Allocate a state structure.
            pSensorData = malloc(queue_sz);
            ESP_LOGI(TAG, "Malloc %d", queue_sz);
            if (pSensorData == NULL)
            {
                ESP_LOGE(TAG, "Cant malloc for sensordata");
                return HTTPD_CGI_DONE;
            }

            // pSensorData->data = malloc(queue_sz);
            // Save the ptr in connData so we get it passed the next time as well.
            connData->cgiData = pSensorData;
            // Set initial pointer to start of string
            // We need to send the headers before sending any data. Do that now.
            httpdStartResponse(connData, 200);
            httpdHeader(connData, "Content-Type", "application/octet-stream");
            httpdEndHeaders(connData);
            uart_flush_input(DEVICE_DATA_UART_NUM);
        }
        else
        {
            const int rxBytes = uart_read_bytes(
                DEVICE_DATA_UART_NUM, pSensorData, queue_sz, 100 / portTICK_RATE_MS);
            if (rxBytes > 0)
            {
                httpdSend(connData, (const char*) pSensorData, queue_sz);
            }
        }

        return HTTPD_CGI_MORE;
    }


    return HTTPD_CGI_DONE;
}

void httpd_task_init()
{
    httpdFreertosInit(&httpdFreertosInstance,
                      builtInUrls,
                      LISTEN_PORT,
                      connectionMemory,
                      MAX_CONNECTIONS,
                      HTTPD_FLAG_NONE);
    httpdFreertosStart(&httpdFreertosInstance);

    ESP_ERROR_CHECK(initCgiWifi());  // Initialise wifi configuration CGI

    printf("\nReady\n");
}
