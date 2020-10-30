/* UART asynchronous example, that uses separate RX and TX tasks

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "protocol_examples_common.h"
#include "cJSON.h"

#include <esp_http_server.h>

static const int RX_BUF_SIZE             = 1024;
static int       data_queue_item_sz      = 0;
static cJSON*    config_json;

#define TXD_HW_UART_PIN (GPIO_NUM_17)
#define RXD_HW_UART_PIN (GPIO_NUM_16)

#define TXD_USB_SERIAL_PIN (GPIO_NUM_4)
#define RXD_USB_SERIAL_PIN (GPIO_NUM_5)
static const char*   TAG                 = "example";

/* An HTTP GET handler */
static esp_err_t config_get_handler(httpd_req_t* req)
{
    if (config_json != NULL)
    {
        const char* resp_str = (const char*) cJSON_Print(config_json);
        httpd_resp_send(req, resp_str, strlen(resp_str));
    }
    else
    {
        const char* resp_str = (const char*) req->user_ctx;
        httpd_resp_send(req, resp_str, strlen(resp_str));
    }

    return ESP_OK;
}

static const httpd_uri_t config_msg = { .uri     = "/config",
                                        .method  = HTTP_GET,
                                        .handler = config_get_handler,
                                        /* Let's pass response string in user
                                         * context to demonstrate it's usage */
                                        .user_ctx = "Configuration not sent from device yet" };

static httpd_req_t* pData_req;
static bool         run_stream = true;
/* An HTTP GET handler */
static esp_err_t stream_get_handler(httpd_req_t* req)
{
    /* Log total visitors */
    ESP_LOGI(TAG, "/stream GET handler send");
    httpd_resp_set_type(req, HTTPD_TYPE_OCTET);
    pData_req = req;
    while (run_stream)
    {
        // if(dataFromDeviceQueue == NULL)
        // {
        //     continue;
        // }
        // else
        // {
        //     uint8_t data_from_queue[data_queue_item_sz];
        //     if(xQueueReceive(dataFromDeviceQueue, data_from_queue, 0))
        //     {
        //         chunk_rsp = httpd_resp_send_chunk(req, (const char*)data_from_queue,
        //         data_queue_item_sz);
        //     }
        // }

        // ESP_LOGI(TAG, "chunk_rsp %d", chunk_rsp);
    }
    ESP_LOGI(TAG, "/stream closed");
    /* Respond with the accumulated value */
    return ESP_OK;
}

static const httpd_uri_t stream
    = { .uri = "/stream", .method = HTTP_GET, .handler = stream_get_handler };
/* This handler allows the custom error handling functionality to be
 * tested from client side. For that, when a PUT request 0 is sent to
 * URI /ctrl, the /hello and /echo URIs are unregistered and following
 * custom error handler http_404_error_handler() is registered.
 * Afterwards, when /hello or /echo is requested, this custom error
 * handler is invoked which, after sending an error message to client,
 * either closes the underlying socket (when requested URI is /echo)
 * or keeps it open (when requested URI is /hello). This allows the
 * client to infer if the custom error handler is functioning as expected
 * by observing the socket state.
 */
esp_err_t http_404_error_handler(httpd_req_t* req, httpd_err_code_t err)
{
    if (strcmp("/config", req->uri) == 0)
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/config URI is not available");
        /* Return ESP_OK to keep underlying socket open */
        return ESP_OK;
    }
    else if (strcmp("/echo", req->uri) == 0)
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/echo URI is not available");
        /* Return ESP_FAIL to close underlying socket */
        return ESP_FAIL;
    }
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &config_msg);
        httpd_register_uri_handler(server, &stream);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}

static void disconnect_handler(void*            arg,
                               esp_event_base_t event_base,
                               int32_t          event_id,
                               void*            event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server)
    {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

static void connect_handler(void*            arg,
                            esp_event_base_t event_base,
                            int32_t          event_id,
                            void*            event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL)
    {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}

void uart_init(void)
{
    const uart_config_t uart_config_qf = {
        .baud_rate  = 460800,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    const uart_config_t uart_config_usb = {
        .baud_rate  = 115200,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config_qf);
    uart_set_pin(
        UART_NUM_1, TXD_HW_UART_PIN, RXD_HW_UART_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uart_driver_install(UART_NUM_0, RX_BUF_SIZE, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config_usb);
    uart_set_pin(
        UART_NUM_0, TXD_USB_SERIAL_PIN, RXD_USB_SERIAL_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}


static bool config_rx = false;

static void rx_task(void* arg)
{
    static const char* connect_str       = "connect";
    static const char* RX_TASK_TAG       = "RX_TASK";
    static int         samples_in_packet = 0;
    static int         num_cols          = 0;
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t*  data      = (uint8_t*) malloc(RX_BUF_SIZE + 1);
    esp_err_t chunk_rsp = ESP_OK;
    while (1)
    {
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 100 / portTICK_RATE_MS);

        if (rxBytes > 0)
        {
            if (!config_rx)
            {
                data[rxBytes] = 0;
                config_json  = cJSON_Parse((char*) data);
                if (config_json != NULL)
                {
                    // ESP_LOGI(
                    //     RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, cJSON_Print(config_json));
                    cJSON* samples_per_packet
                        = cJSON_GetObjectItemCaseSensitive(config_json, "samples_per_packet");
                    cJSON* column_location
                        = cJSON_GetObjectItemCaseSensitive(config_json, "column_location");
                    if (cJSON_IsNumber(samples_per_packet))
                    {
                        samples_in_packet = samples_per_packet->valueint;
                        num_cols          = cJSON_GetArraySize(column_location);
                        ESP_LOGI(RX_TASK_TAG,
                                 "Samples in packet %d cols %d",
                                 samples_in_packet,
                                 num_cols);
                        data_queue_item_sz = samples_in_packet * sizeof(int16_t) * num_cols;
                    }
                    uart_tx_chars(UART_NUM_1, connect_str, strlen(connect_str));
                    uart_wait_tx_done(UART_NUM_1, 1000 / portTICK_RATE_MS);
                    config_rx = true;
                    uart_flush_input(UART_NUM_1);
                }
            }
            else
            {
                if (run_stream && pData_req != NULL)
                {
                    chunk_rsp
                        = httpd_resp_send_chunk(pData_req, (const char*) data, data_queue_item_sz);
                    ESP_LOGI(TAG, "chunk_rsp %d", chunk_rsp);
                    run_stream = chunk_rsp == ESP_OK;
                }
                continue;  // ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
            }
        }
    }
    free(data);
}

void app_main(void)
{
    static httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    /* Register event handlers to stop the server when Wi-Fi or Ethernet is disconnected,
     * and re-start it upon connection.
     */
#ifdef CONFIG_EXAMPLE_CONNECT_WIFI
    ESP_ERROR_CHECK(
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
#endif  // CONFIG_EXAMPLE_CONNECT_WIFI
#ifdef CONFIG_EXAMPLE_CONNECT_ETHERNET
    ESP_ERROR_CHECK(
        esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(
        ETH_EVENT, ETHERNET_EVENT_DISCONNECTED, &disconnect_handler, &server));
#endif  // CONFIG_EXAMPLE_CONNECT_ETHERNET

    /* Start the server for the first time */
    // #define xQueueCreate( uxQueueLength, uxItemSize ) xQueueGenericCreate( ( uxQueueLength ), (
    // uxItemSize ), ( queueQUEUE_TYPE_BASE ) )
    uart_init();
    xTaskCreate(rx_task, "uart_rx_task", 1024 * 2, NULL, configMAX_PRIORITIES-1, NULL);
    // xQueueCreate()
    server = start_webserver();
}
