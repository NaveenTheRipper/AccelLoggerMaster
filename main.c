#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SparkFunADXL313.h>       // ADXL313 Accelerometer
#include <SparkFun_RV8803.h>       // RV-8803 RTC
#include "SdFat.h"                 // FAT filesystem for SD card

#include "esp_wifi.h"
#include "esp_system.h"
#include "driver/adc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define error(msg) sd.errorHalt(F(msg))

// Sensor & Peripheral Objects
ADXL313 myAdxl;
RV8803 rtc;
SdFat sd;
SdFile file;

// SD card chip select pin
const int chipSelect = 5;

// Filenames and formatting buffers
char gpsDate[64];
char header[32];
char Date[32];
char cap[32];

// Time variables
int mint, sect, hourt, dayt, montht, yeart;

// NTP server (not actively used here)
const char* ntpServer = "pool.ntp.org";

// Task handle
TaskHandle_t Task1;

// Function to update the Date/Time from RTC
void updatetime() {
  rtc.updateTime();
  dayt = rtc.getDate();
  montht = rtc.getMonth();
  yeart = rtc.getYear();
  mint = rtc.getMinutes();
  sect = rtc.getSeconds();
  hourt = rtc.getHours();
  sprintf(Date, "%d/%d/%d %02d:%02d:%02d", yeart, montht, dayt, hourt, mint, sect);
}

// Task that runs on Core 0 to read and log accelerometer data
void Task1code(void * pvParameters) {
  for (;;) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = 1; // Run task every 1 tick

    if (myAdxl.dataReady()) {
      myAdxl.readAccel(); // Read accelerometer data

      updatetime(); // Get current date and time

      // Format data as CSV
      sprintf(gpsDate, "%s,%d,%d,%d,\n", Date, int(myAdxl.x), int(myAdxl.y), int(myAdxl.z));

      ofstream sdout(cap, ios::out | ios::app);
      if (!sdout) {
        error("open failed");
      }

      sdout << gpsDate;
      sdout.close();
    }

    // Scheduled ESP restart at specific times
    if ((hourt == 1 && mint == 15 && sect == 6) ||
        (hourt == 6 && mint == 11 && sect == 6) ||
        (hourt == 6 && mint == 13 && sect == 6) ||
        (hourt == 6 && mint == 14 && sect == 6)) {
      ESP.restart();
    }

    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void setup() {
  // Reduce power consumption
  adc_power_off();
  esp_wifi_stop();

  // Start I2C and Serial
  Wire.begin();
  Serial.begin(115200);

  // Lower CPU frequency to save power
  setCpuFrequencyMhz(80);

  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("RTC not detected. Check wiring.");
    while (1);
  }

  rtc.set24Hour();
  rtc.updateTime();
  updatetime();

  // Prepare filename
  sprintf(cap, "/test%d%d%d.csv", yeart, montht, dayt);
  Serial.println(cap);

  // Initialize accelerometer
  if (!myAdxl.begin()) {
    Serial.println("ADXL313 not detected. Check wiring.");
    while (1);
  }

  myAdxl.standby();                     // Put into standby to configure
  myAdxl.setRange(ADXL313_RANGE_2_G);  // ±2g range
  myAdxl.setBandwidth(ADXL313_BW_50);  // 50 Hz
  myAdxl.setFullResBit(true);          // Enable full resolution
  myAdxl.measureModeOn();              // Begin measurement

  // Initialize SD card
  if (!sd.begin(chipSelect, SD_SCK_MHZ(10))) {
    sd.initErrorHalt();
  }

  // Create and write CSV header if file doesn't exist
  if (!sd.exists(cap)) {
    if (!file.open(cap, O_WRONLY | O_CREAT | O_EXCL)) {
      error("file.open failed");
    }

    sprintf(header, "%s,%s,%s,%s,", "Date", "X", "Y", "Z");
    file.print(F(header));
    file.println();
    file.close();
  }

  // Start FreeRTOS Task on Core 0
  xTaskCreatePinnedToCore(
    Task1code,      // Task function
    "Task1",        // Name of task
    20000,          // Stack size
    NULL,           // Task parameters
    1,              // Priority
    &Task1,         // Task handle
    0);             // Core 0
}

void loop() {
  // All logic handled by FreeRTOS Task
}
