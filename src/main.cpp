// GxEPD2__with_two_fonts.ino by Henry Leach, adapted from
// GxEPD2_HelloWorld.ino by Jean-Marc Zingg
// For testing on Wavesshares 2.9" epaper display.
// www.henryleach.com

// Wiring for AVR, UNO, NANO etc.
// BUSY -> 7, RST -> 9, DC -> 8, CS-> 10, CLK -> 13, DIN -> 11
#include <Arduino.h>
#include <secrets.h> // Include your secrets.h file with the following variables defined:
// WIFI_SSID, WIFI_PASSWORD, INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN

#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#define DEVICE "ESP8266"
#endif

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
#include <Fonts/FreeSansBold24pt7b.h> //Large font uses a lot of memory
//#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <FreeSansBold32pt7b.h>

// select the display class and display driver class in the following file (new style):
#include "GxEPD2_display_selection_new_style.h"

//Define global constants and variables
const char greeting[] = "Solar";
const uint16_t refreshrate = 5000;
int16_t tbx, tby;
uint16_t tbw, tbh;

double_t ACpower = 0;
double_t ACtotal = 0;
double_t ACdaily = 0;

// Print initial message in large text across the top of the screen
void drawGreeting()
{
  display.setFont(&FreeSansBold24pt7b); //This almost fills the screen with "Wednesday"
  display.setTextColor(GxEPD_BLACK, GxEPD_WHITE);
  display.getTextBounds(greeting, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center the bounding box by transposition of the origin:
  uint16_t x1 = ((display.width() - tbw) / 2) - tbx;  // centres the text
  uint16_t y1 = ((display.height() - tbh) / 4) - tby; // Aligned in top half
  display.setFullWindow();
  display.firstPage();
  do // Print the upper part of the screen
  {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(x1, y1);
    display.print(greeting);
  }
  while (display.nextPage());

  //sent display to sleep
  display.hibernate();
}

// Print actual data
void drawRefresh(){

  //char countText[8];
  //snprintf(countText, sizeof(countText), "%3.1f W", ACpower);

  // Fonts need scaling factor 1.333 compared to GIMP sketch
  display.setFont(&FreeSansBold32pt7b);
  //display.setTextSize(2);
  display.setTextColor(GxEPD_BLACK, GxEPD_WHITE);
  //display.getTextBounds(countText, 0, 0, &tbx, &tby, &tbw, &tbh);
  //uint16_t x2 = ((display.width() - tbw) * 1 / 4) - tbx;
  //uint16_t y2 = ((display.height() - tbh) * 1 / 4) - tby;
  //args are(x-start, y-start, x-end, y-end)
  //absolute on display. This covers the top half.
  display.setPartialWindow(0, 0, display.width(), display.height() / 2);
  //display.setFullWindow();
  display.firstPage();
  do {
    // display.fillScreen(GxEPD_WHITE);
    //display.setCursor(x2, y2);
    //display.setTextSize(2);
    display.setCursor(0, 79);
    display.printf("%3.0f", ACpower); //needs 32pt fonts for correct scaling, workaround: use 18pt + scaling factor 2
    display.setFont(&FreeSans18pt7b);
    //display.setTextSize(1);
    display.setCursor(104, 79);
    display.print("W");
    display.setFont(&FreeSans12pt7b); //12pt instead of 9 pt = factor 1.3333
    display.setCursor(0, 17);
    display.print("Leistung");
    // display.drawRect(0, display.height() / 2, display.width(), display.height(), GxEPD_BLACK);
  }
  while (display.nextPage());
  // and put the display to sleep, even if it's only for a short time
  display.powerOff();
  display.hibernate();
  // delay(refreshrate);
}

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
      ACpower = value;
    }
    
    if (feature == "AC-daily")
    {
      ACdaily = value;
    }
    
    if (feature == "AC-total")
    {
      ACtotal = value;
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

  // Print actual values on display
  drawRefresh();

  // Wait for length of $refresharate
  Serial.printf("Wait %i s\n", refreshrate);
  delay(refreshrate);
}