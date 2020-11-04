#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "cJSON.h"
#include "uart_task.h"
#include "httpd_task.h"

static int    data_queue_item_sz = 0;
static cJSON* config_json;
static bool   config_rx = false;

int get_data_queue_size() { return data_queue_item_sz; }

cJSON* get_config_json() { return config_json; }

void uart_init(void)
{
    const uart_config_t uart_config_qf = {
        .baud_rate  = 115200 *4,
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
    uart_driver_install(DEVICE_DATA_UART_NUM, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(DEVICE_DATA_UART_NUM, &uart_config_qf);
    uart_set_pin(DEVICE_DATA_UART_NUM,
                 TXD_HW_UART_PIN,
                 RXD_HW_UART_PIN,
                 UART_PIN_NO_CHANGE,
                 UART_PIN_NO_CHANGE);

    uart_driver_install(USB_SERIAL_UART_NUM, RX_BUF_SIZE, 0, 0, NULL, 0);
    uart_param_config(USB_SERIAL_UART_NUM, &uart_config_usb);
    uart_set_pin(USB_SERIAL_UART_NUM,
                 TXD_USB_SERIAL_PIN,
                 RXD_USB_SERIAL_PIN,
                 UART_PIN_NO_CHANGE,
                 UART_PIN_NO_CHANGE);
}

uint8_t num_stored_packets;

void uart_task_rx(void* arg)
{
    static int samples_in_packet = 0;
    static int num_cols          = 0;
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE + 1);

    while (1)
    {
        if (!config_rx)
        {
            const int rxBytes
                = uart_read_bytes(DEVICE_DATA_UART_NUM, data, RX_BUF_SIZE, 100 / portTICK_RATE_MS);

            if (rxBytes > 0)
            {
                data[rxBytes] = 0;
                config_json   = cJSON_Parse((char*) data);
                if (config_json != NULL)
                {
                    cJSON* samples_per_packet
                        = cJSON_GetObjectItemCaseSensitive(config_json, "samples_per_packet");
                    cJSON* column_location
                        = cJSON_GetObjectItemCaseSensitive(config_json, "column_location");
                    if (cJSON_IsNumber(samples_per_packet))
                    {
                        samples_in_packet  = samples_per_packet->valueint;
                        num_cols           = cJSON_GetArraySize(column_location);
                        data_queue_item_sz = samples_in_packet * sizeof(int16_t) * num_cols;
                        ESP_LOGI(RX_TASK_TAG,
                                 "Samples in packet %d cols %d queue_sz %d",
                                 samples_in_packet,
                                 num_cols,
                                 data_queue_item_sz);
                        // for (int i = 0; i < MAX_SENSOR_DATA_MSG; i++)
                        // {
                        //     dataMessages[i].index = i;
                        //     dataMessages[i].data  = malloc(data_queue_item_sz);
                        // }
                        // uart_data_queue
                        //     = xQueueCreate(MAX_SENSOR_DATA_MSG, sizeof(struct sensorDataMessage));
                        // if (uart_data_queue == NULL)
                        // {
                        //     ESP_LOGE(RX_TASK_TAG, "Couldn't create data queue!");
                        // }
                    }
                    uart_tx_chars(DEVICE_DATA_UART_NUM, CONNECT_STR, strlen(CONNECT_STR));
                    uart_wait_tx_done(DEVICE_DATA_UART_NUM, 1000 / portTICK_RATE_MS);
                    config_rx = true;
                    free(data);
                    uart_flush_input(DEVICE_DATA_UART_NUM);
                    vTaskDelete(NULL);
                }
            }
        }
        // else
        // {
        //     // BaseType_t q_resp;
        //     // const int rxBytes = uart_read_bytes(DEVICE_DATA_UART_NUM,
        //     //                                     dataMessages[num_stored_packets].data,
        //     //                                     data_queue_item_sz,
        //     //                                     10 / portTICK_RATE_MS);
        //     // if (rxBytes > 0 && httpd_task_is_stream_open())
        //     // {
        //     //     xQueueSend(
        //     //         uart_data_queue, &dataMessages[num_stored_packets], 100 / portTICK_RATE_MS);b
        //     //     {
        //     //         num_stored_packets++;
        //     //     }
        //     //     if (num_stored_packets == MAX_SENSOR_DATA_MSG)
        //     //     {
        //     //         num_stored_packets = 0;
        //     //     }
        //         vTaskDelay(100);
        //     // }
        // }
    }
}
