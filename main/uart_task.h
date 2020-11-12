#ifndef __UART_TASK_H__
#define __UART_TASK_H__
#include "driver/gpio.h"
#include "cJSON.h"
#include "freertos/queue.h"

#define CONNECT_STR "connect"
#define RX_TASK_TAG "RX_TASK"
#define RX_BUF_SIZE 1024

#define TXD_HW_UART_PIN (GPIO_NUM_17)
#define RXD_HW_UART_PIN (GPIO_NUM_16)
#define DEVICE_DATA_UART_NUM UART_NUM_1

#define TXD_USB_SERIAL_PIN (GPIO_NUM_4)
#define RXD_USB_SERIAL_PIN (GPIO_NUM_5)
#define USB_SERIAL_UART_NUM UART_NUM_0

#define MAX_SENSOR_DATA_MSG 50

extern bool results_ready;

void   uart_task_rx(void* arg);
void   uart_init(void);
int    get_data_queue_size();
cJSON* get_config_json();
cJSON* get_results_json();


#endif  //__UART_TASK_H__
