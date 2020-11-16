# ESP32 UART to HTTP bridge

ESP32 UART to HTTP bridge for simple streaming interface. This application will communicate on UART1 to a target device at 460800bps (configurable)

It will then create a `/config` and `/stream` endpoints for get the device configuration and stream data to the Data Capture Libraries

It is assumed that you will have the [ESP-IDF](https://github.com/espressif/esp-idf) set up on your machine already.

## Cloning this repository

To clone this repository, make sure it is cloned with the `--recursive` option. `git clone https://github.com/sensiml/esp32_simple_http_uart.git --recursive`

## Configuring WiFi SSID/Password

Run the ESP-IDF menuconfig tool: `idf.py menuconfig`, and configure the `Example Connection Configuration` section with your SSID/Passphrase.

## Swapping UART Pins

If you are connecting two feather boards together, you will need to swap the UART RX/TX Pins. This can be done by setting `RX_TX_NEEDS_SWAP` in `uart_task.h` to 1.

## Other Tips

It is useful to have the UART ISR configured to be placed in RAM, but it is not necessary. This is done through the menuconfig.

## Recognition results

The application will assume that if it gets valid JSON over a serial port, and it isn't a capture configuration, that it is getting recognition results.

These results will be viewable at the `/results` endpoint. `/config` and `/stream` endpoints won't work when streaming results.

# Using with Nano33 or other devices.

SensiML's firmware for the [Arduino Nano33](https://github.com/sensiml/nano33_data_capture) is set up to communicate with this firmware using the second hardware UART on the Nano33. However, the cables aren't necessarily meant to line up. Below, an image is shown with a breadboard to illustrate the wiring needed to power the Nano33 from the ESP32 feather with a battery.

![ESP32 Feather and Nano33 on a Breadboard](images/esp32_nano33_breadboard.jpg)

