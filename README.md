# ESP32-S3-HMI-V3

## Hardware
●ESP32-S3 dual-core Xtensa 32-bit LX7 microprocessor, up to 240 MHz with 384KB ROM, 512KB SRAM. 2.4GHz WiFi and Bluetooth 5
●PSRAM: 2MB     FLASH: 8MB
●Storage: Micro-SD card slot
●Display: 4.3-inch display with 800×480 resolution, I2C capacitive touch panel
●Display interface: 16-bits, 8080 parallel communication
●Audio: Codec, Audio amplifier, built-in microphone, speaker
●USB: 1x USB-C OTG (DFU/CDC) port, 1x USB-C debug port
●Sensors: 3-axis accelerometer, 3-axis gyroscope, temperature and humidity sensors
●Peripheral interface: I2C, SPI, GPIOs
●Miscellaneous: wakeup and reset buttons, power switch
●Power Supply: 5V / 1A
●Dimension: 110 x 61 x 13.5mm

![B01-en](https://user-images.githubusercontent.com/10337553/179403894-12db0b61-64a1-4383-a967-e4f00776ee1e.png)

![B00-800x800](https://user-images.githubusercontent.com/10337553/179403909-1f0d9f97-f844-46a2-9fde-c18f916608db.png)

## Software
All the examples are stored in .../examples folder. Please build it with **ESP-IDF 4.4.2**
![f348cb5e-235e-468a-8372-5e6ca2c9965f](https://github.com/W00ng/ESP32-S3-HMI-V3/assets/10337553/a69f3fcd-1425-4838-94a4-f8502d2b7bf5)

### Step 1: Enter the examples folder
Open the terminal and go to any folder that stores examples (e.g. lvgl_demo):

```bash
cd ...\examples\lvgl_demo
```

### Step 2: Build the example

```bash
idf.py build
```

### Step 3: Flash and launch monitor
Flash the program and launch IDF Monitor:

```bash
idf.py flash monitor
```

## Support

if you need any help, please contact: aemails@163.com
https://www.youtube.com/watch?v=Nls3AOx-OM4

