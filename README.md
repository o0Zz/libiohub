# libiohub

> **A lightweight C library for modular IoT driver abstraction across ESP32, Arduino, and Raspberry Pi.**

`libiohub` provides a unified interface for SPI, I²C, and UART communication and a flexible way to develop and integrate hardware drivers such as climate systems, lighting controllers, sensors...
It is designed to simplify cross-platform IoT development using clean, platform-agnostic C interfaces.


## 🚀 Features

- **Unified bus abstraction** — consistent SPI, I²C, and UART APIs across platforms  
- **Modular driver system** — plug in multiple device types and brands (e.g., Mitsubishi, Toshiba, Midea, Philips, ...)  
- **Platform support** — ESP32, Arduino, and Raspberry Pi via dedicated backends  
- **Pure C implementation** — minimal dependencies, lightweight footprint  
- **Portable structure** — easy to extend with new drivers or platforms  


## 🧩 Example Usage

Example: using a Mitsubishi climate driver via the unified API.

```c
#include "iohub.h"
#include "heatpump/mitsubishi.h"
#include <stdio.h>

int main(void)
{
    // Contexts for UART and Mitsubishi heatpump driver
    iohub_uart_t uart_ctx;
    iohub_heatpump_mitsubishi_t mitsubishi_ctx;

    // Initialize UART interface (example: UART1, TX pin 1, RX pin 2)
    ret_code_t err = iohub_uart_init(&uart_ctx, 1, 2);
    IOHUB_ASSERT(err == SUCCESS, "UART initialization failed");

    // Initialize Mitsubishi heatpump driver with UART backend
    err = iohub_heatpump_mitsubishi_init(&mitsubishi_ctx, &uart_ctx);
    IOHUB_ASSERT(err == SUCCESS, "Mitsubishi driver initialization failed");

    // Send command: turn ON, set 24°C, auto fan, cooling mode
    err = iohub_heatpump_mitsubishi_send(
        &mitsubishi_ctx,
        HEATPUMP_ACTION_ON,
        24.0f,
        HEATPUMP_FAN_AUTO,
        HEATPUMP_MODE_COOL
    );
    IOHUB_ASSERT(err == SUCCESS, "Failed to send Mitsubishi command");

    printf("Mitsubishi heatpump command sent successfully!\n");

    return 0;
}
```

## Prerequisite
    - Linux/Windows FTDI Driver: https://ftdichip.com/drivers/vcp-drivers/

## ⚙️ Building

You can build libiohub with CMake or integrate it directly into your own project’s build system.

Example (CMake)
```
mkdir build && cd build
cmake ..
make
```

To build for a specific target (e.g. ESP32):

```
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/esp32.cmake ..
make
```
## Using this library on Arduino

In order to use this library on arduino you need to put this library in the `Documents\Arduino\libraries`

```
	mkdir "%USERPROFILE%\Documents\Arduino\libraries"
	cd "%USERPROFILE%\Documents\Arduino\libraries"
	git clone https://github.com/o0Zz/libiohub.git --recursive
```

In your ino file:

```
#include "iohub.h"
```

Then start using it


## 🧱 Adding a New Driver

Create a new header and source file under the appropriate module (e.g. include/climate/daikin.h, src/climate/daikin.c)

Implement the iohub_climate_driver_t interface

## 🧰 Supported Platforms
|Platform    |SPI|I²C|UART|Notes|
|------------|---|---|----|-----|
|ESP32       |✅ |✅  |✅  |Uses ESP-IDF HAL|
|Arduino     |✅ |✅  |✅  |Standard Arduino APIs|
|Raspberry Pi|✅ |✅  |✅  |Uses WiringPi / BCM2835|
|Win/Linux (MPSSE)|✅ |✅  |✅  |Uses FTDI driver|