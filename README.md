# 3-axis Acceleration Data Acquisition System

This project logs 3-axis acceleration data from an ADXL313 accelerometer to a microSD card, timestamped using an RV-8803 RTC. It is powered by an ESP32 using FreeRTOS to offload the logging task to a dedicated core.

## Hardware
- ESP32
- ADXL313 (I2C)
- RV-8803 RTC(I2C)
- microSD card module (SPI)
- Optional: Battery backup for RTC

## Features
- Real-time timestamped data logging
- FreeRTOS task scheduling
- Daily CSV file creation
- Periodic self-reset capability

