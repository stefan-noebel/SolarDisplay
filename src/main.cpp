// GxEPD2__with_two_fonts.ino by Henry Leach, adapted from
// GxEPD2_HelloWorld.ino by Jean-Marc Zingg
// For testing on Wavesshares 2.9" epaper display.
// www.henryleach.com

// Wiring for AVR, UNO, NANO etc.
// BUSY -> 7, RST -> 9, DC -> 8, CS-> 10, CLK -> 13, DIN -> 11
#include <Arduino.h>
#include <esp_sleep.h>
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

//Define global constants and variables
const char greeting[] = "Solar";
const uint16_t refreshrate = 10; // Refresh rate in seconds, set to 10 s for testing, change to 600 for 10 min updates
int16_t tbx, tby;
uint16_t tbw, tbh;

//double_t ACpower = 0;
//double_t ACtotal = 0;
//double_t ACdaily = 0;

struct SolarData {
    double_t ACpower = 0;
    double_t ACtotal = 0;
    double_t ACdaily = 0;
};

SolarData ACdata;

// Print actual data
void drawRefresh(SolarData &data){

  // next steps
  // ~~~(0) Secrets sicher abspeichern~~~
  // ~~~(1) statisch und dynamisch trennen -> 2 Funktionen für unterschiedlichen Aufruf~~~
  // -> funtioniert nicht, da partial refresh nur für Koordinaten mit ganzahlige Vielfachen von 8 möglich sind
  // ~~~(1) Funktion fetchinfluxdb ausgliedern~~~
  // (2) Code bereinigen und auf Github etc.
  // (2) Serial: nützliche Debug-Ausgaben und Grunddaten 
  // ~~~(3) Deepsleep ausprobieren -> kein loop mehr sondern nur setup~~~
  // ~~~(3) Upload mode per PIN setzen, um dfu-util nutzen zu können~~~
  // (4) Ladeschaltung verbinden

  //char countText[8];
  //snprintf(countText, sizeof(countText), "%3.1f W", ACpower);
  // Cacluclate bars sizes
  tbh = nearbyint(data.ACpower / 600 * 94);
  tbw = nearbyint(data.ACdaily / 5 * 198);

  // Fonts need scaling factor 1.333 compared to GIMP sketch
  //display.setFont(&FreeSansBold32pt7b);
  //display.setTextSize(2);
  display.setTextColor(GxEPD_BLACK, GxEPD_WHITE);
  //display.getTextBounds(countText, 0, 0, &tbx, &tby, &tbw, &tbh);
  //uint16_t x2 = ((display.width() - tbw) * 1 / 4) - tbx;
  //uint16_t y2 = ((display.height() - tbh) * 1 / 4) - tby;
  //args are(x-start, y-start, x-end, y-end)
  //absolute on display. This covers the top half.
  display.setPartialWindow(0, 0, display.width(), display.height());
  //display.setFullWindow();
  display.firstPage();
  do {
    //
    // dynamic text
    // display.fillScreen(GxEPD_WHITE);
    //display.setCursor(x2, y2);
    //display.setTextSize(2);
    display.setFont(&FreeSansBold32pt7b);
    display.setCursor(0, 79);
    display.printf("%3.0f", data.ACpower); //needs 32pt fonts for correct scaling, workaround: use 18pt + scaling factor 2
    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(68, 128);
    display.printf("%1.1f", data.ACdaily);
    display.setCursor(68, 199);
    display.printf("%4.0f", data.ACtotal);
    //
    // dynamic bars
    display.fillRect(165, 95 - tbh, 32, tbh, GxEPD_BLACK);
    display.fillRect(1, 133, tbw, 14, GxEPD_BLACK);
    //
    // static text
    display.setFont(&FreeSans18pt7b);
    //display.setTextSize(1);
    display.setCursor(106, 79);
    display.print("W");
    display.setFont(&FreeSans12pt7b); //12pt instead of 9 pt = factor 1.3333
    display.setCursor(0, 17);
    display.print("Leistung");
    display.setCursor(124, 17);
    display.print("600");
    display.setCursor(150, 95);
    display.print("0");
    display.drawRect(164, 0, 35, 96, GxEPD_BLACK);
    display.drawRect(0, 99, 200, 2, GxEPD_BLACK);
    display.drawRect(0, 132, 200, 16, GxEPD_BLACK);
    display.drawRect(0, 171, 200, 2, GxEPD_BLACK);
    display.setCursor(0, 128);
    display.print("heute");
    display.setCursor(117, 128);
    display.print("kWh");
    display.setCursor(0, 167);
    display.print("0");
    display.setCursor(188, 167);
    display.print("5");
    display.setCursor(0, 199);
    display.print("total");
    display.setCursor(146, 199);
    display.print("kWh");
  }
  while (display.nextPage());
  // and put the display to sleep, even if it's only for a short time
  display.powerOff();
  display.hibernate();
  // delay(refreshrate);
}

void fetchInfluxDB(SolarData &data)
{
  // Construct a Flux query
  // Query will list RSSI for last 24 hours for each connected WiFi network of this device type
  // String query = "from(bucket: \"" INFLUXDB_BUCKET "\") |> range(start: -1h)";
  String query = "from(bucket: \"" INFLUXDB_BUCKET "\") |> range(start: -24h) \
     |> filter(fn: (r) => r._measurement == \"solar\") \
     |> filter(fn: (r) => r.device == \"Solar\") \
     |> filter(fn: (r) => r.feature =~ /^AC-*/) \
     |> last()";
 
  Serial.println("==== List results ====");
 
  // Print composed query
  Serial.print("Querying with: ");
  Serial.println(query);
  Serial.println();
 
  // Send query to the server and get result
  FluxQueryResult result = client.query(query);
 
  // Print only value and timestamp as table
  //Iterate over rows. Even there is just one row, next() must be called at least once.
  while (result.next()) {
    // Extract values from results
    //for(FluxValue &val: result.getValues()) {
    //  // Check whether the value is null
    //  if(!val.isNull()) {
    //    // Use raw string, unconverted value
    //    Serial.print(val.getRawValue());
    //    Serial.print("  ");
    //  }
    //}
    //Serial.println();
    // Fallunterscheidung nach AC-power, AC-daily und AC-total unter feature
    String feature = result.getValueByName("feature").getString();
    Serial.print(feature);
    Serial.print(":  ");
    double value = result.getValueByName("_value").getDouble();
    Serial.print(value);

    // Get converted value for the _time column
    FluxDateTime ACtime = result.getValueByName("_time").getDateTime();
 
    // Format date-time for printing
    // Format string according to http://www.cplusplus.com/reference/ctime/strftime/
    String timeStr = ACtime.format("%T");

    Serial.print(" at ");
    Serial.print(timeStr);

    Serial.println();

    if (feature == "AC-power")
    {
      data.ACpower = value; // in W
    }
    
    if (feature == "AC-daily")
    {
      data.ACdaily = value; // in kWh
    }
    
    if (feature == "AC-total")
    {
      data.ACtotal = value; // in kWh
    }
  }
  Serial.println();


  // Check if there was an error
  if (result.getError().length() > 0) {
    Serial.print("Query result error: ");
    Serial.println(result.getError());
  }

  // Close the result
  result.close();
}

// Run once
void setup()
{
  Serial.begin(115200);
  pinMode(D2, INPUT_PULLUP);

  // Check for upload mode before deep sleep
  if (digitalRead(D2) == LOW) {
    delay(5000);
    Serial.println("UPLOAD MODE: Staying awake 30 s for firmware upload/debug.");
    for (size_t i = 0; i < 30; i++)
    {
      Serial.print("."); // Print dots to indicate that the device is awake
      delay(1000);
    }
    Serial.println();
  }
 
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
  timeSync(TZ_INFO, "fritz.box", "pool.ntp.org");
 
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
  
  // Fetch data from InfluxDB
  fetchInfluxDB(ACdata);

  // Disconnect WiFi to save power
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  // Print actual values on display
  drawRefresh(ACdata);

  // Print and enter deep sleep
  Serial.printf("Going to deep sleep for %i s\n", refreshrate);
  esp_sleep_enable_timer_wakeup((uint64_t)refreshrate * 1000 * 1000); // refreshrate is in s; sleeping time is in us
  // Ensure all serial output is sent
  Serial.flush();
  esp_deep_sleep_start();
}

void loop() {
  // Not used with deep sleep
}