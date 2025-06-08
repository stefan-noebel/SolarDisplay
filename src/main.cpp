// GxEPD2__with_two_fonts.ino by Henry Leach, adapted from
// GxEPD2_HelloWorld.ino by Jean-Marc Zingg
// For testing on Wavesshares 2.9" epaper display.
// www.henryleach.com

// Wiring for AVR, UNO, NANO etc.
// BUSY -> 7, RST -> 9, DC -> 8, CS-> 10, CLK -> 13, DIN -> 11
#include <Arduino.h>
#include <secrets.h> // Include secrets.h for WiFi and InfluxDB credentials


// Include ESP32 and WiFi libraries
#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#define DEVICE "ESP8266"
#endif

// Include Influx Client library
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

// Set timezone string according to https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
// Examples:
//  Pacific Time: "PST8PDT"
//  Eastern: "EST5EDT"
//  Japanesse: "JST-9"
//  Central Europe: "CET-1CEST,M3.5.0,M10.5.0/3"
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"
 
// InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// Include ePaper Display library
#include <GxEPD2_BW.h> // including both doesn't use more code or ram
#include <GxEPD2_3C.h> // including both doesn't use more code or ram

// Use two fonts for two different text sizes. 
//#include <Fonts/FreeSansBold24pt7b.h> //Large font uses a lot of memory
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
//#include <Fonts/FreeSans9pt7b.h>
#include <FreeSansBold32pt7b.h>

// select the display class and display driver class in the following file (new style):
#include "GxEPD2_display_selection_new_style.h"

// Include local header files
#include "display_utils.h"
#include "influxdb_utils.h"

//Define global constants and variables
const char greeting[] = "Solar";
const uint16_t refreshrate = 5000;
int16_t tbx, tby;
uint16_t tbw, tbh;

double_t ACpower = 0;
double_t ACtotal = 0;
double_t ACdaily = 0;

// Run once
void setup()
{
  Serial.begin(115200);
 
  // Setup wifi
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
 
  Serial.print("Connecting to wifi");
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
 
  // Accurate time is necessary for certificate validation
  // For the fastest time sync find NTP servers in your area: https://www.pool.ntp.org/zone/
  // Syncing progress and the time will be printed to Serial
  timeSync(TZ_INFO, "fritz.box");
 
  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
  
  display.init(115200, true, 2, false);
  display.setRotation(1); //0 is 'portrait'
  
  // draw full screen greeter first
  drawGreeting();
}

void loop() {
  // Fetch data from InfluxDB
  fetchInfluxDB();

  // Print actual values on display
  drawRefresh();

  // Wait for length of $refresharate
  Serial.printf("Wait %i s\n", refreshrate);
  delay(refreshrate);
}