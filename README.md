# ESP32 UART to HTTP bridge

ESP32 UART to HTTP bridge for simple streaming interface. This application will communicate on UART1 to a target device at 460800bps (configurable)

It will then create a `/config` and `/stream` endpoints for get the device configuration and stream data to the Data Capture Libraries

It is assumed that you will have the [ESP-IDF](https://github.com/espressif/esp-idf) set up on your machine already.

## Cloning this repository

To clone this repository, make sure it is cloned with the `--recursive` option. `git clone https://github.com/sensiml/esp32_simple_http_uart.git --recursive`

## Configuring WiFi SSID/Password

Run the ESP-IDF menuconfig tool: `idf.py menuconfig`, and configure the `Example Connection Configuration` section with your SSID/Passphrase.

![Example Connection Configuration](images/example_connection_config.png)

![Example Connection SSID and Password](images/example_connection_ssid_pw.png)

## UART Configuration

In the ESP-IDF menuconfig tool, you will see `Application UART Configuration`

![Application UART Configuration in menuconfig](images/application_uart_configuration.png)

### Swapping UART Pins

If you are connecting two feather boards together, you will need to swap the UART RX/TX Pins. Select `Data UART Swap RX/TX` to do this.

![Application UART swap pins](images/application_uart_swap.png)

### Changing Data UART Baud Rate

The application defaults to `480600 bps` as a baud rate for the UART. This can be configured in the configuration.

Select `Data UART Baud Rate`

![Application UART rate](images/application_uart_rate.png)

And choose from the list of options

![Application UART rate options](images/application_uart_rate_options.png)

### Flashing The Board

```bash
idf.py build
idf.py -p [PORT] flash
```

### Getting the IP Address

```bash
idf.py -p [PORT] monitor
```

Look for the static ip address (sta ip) to know where to connect

```bash
> I (3115) esp_netif_handlers: sta ip: 192.168.86.249, mask: 255.255.255.0, gw: 192.168.86.1
```

## Other Tips

It is useful to have the UART ISR configured to be placed in RAM, but it is not necessary. This is done through the menuconfig.

## Recognition results

The application will assume that if it gets valid JSON over a serial port, and it isn't a capture configuration, that it is getting recognition results.

These results will be viewable at the `/results` endpoint. `/config` and `/stream` endpoints won't work when streaming results.

# Using with QuickFeather (and other Feather Boards)

Using the [Huzzah32 Feather](https://www.adafruit.com/product/3619) with stacking headers installed, insert another feather/wing board into the headers to connect them. Shown here is the [QuickLogic QuickFeather](https://www.quicklogic.com/products/eos-s3/quickfeather-development-kit/) on top.

**Note:** When using two feather boards, you will need to [Swap UART pins](#Swapping-UART-Pins) in order for communication to work properly.

![QuickLogic QuickFeather stacked on top of Huzzah32](images/qf_esp32_stack.png)

When using a battery, use the battery terminal of the Huzzah32 Feather.
![QuickLogic QuickFeather stacked on top of Huzzah32](images/qf_esp32_stack_battery.png)

# Using with Nano33 or other devices.

SensiML's firmware for the [Arduino Nano33](https://github.com/sensiml/nano33_data_capture) is set up to communicate with this firmware using the second hardware UART on the Nano33. However, the cables aren't necessarily meant to line up. Below, an image is shown with a breadboard to illustrate the wiring needed to power the Nano33 from the ESP32 feather with a battery.

![ESP32 Feather and Nano33 on a Breadboard](images/esp32_nano33_breadboard.jpg)
