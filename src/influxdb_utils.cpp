#include "influxdb_utils.h"
#include <InfluxDbClient.h>
#include <Arduino.h>
#include "secrets.h" // Include secrets.h for WiFi and InfluxDB credentials

// Extern variables from main.cpp
extern double_t ACpower, ACtotal, ACdaily;
extern InfluxDBClient client;

void fetchInfluxDB()
{
    String query = "from(bucket: \"" INFLUXDB_BUCKET "\") |> range(start: -24h) "
                   "|> filter(fn: (r) => r._measurement == \"solar\") "
                   "|> filter(fn: (r) => r.device == \"Solar\") "
                   "|> filter(fn: (r) => r.feature =~ /^AC-*/) "
                   "|> last()";

    Serial.println("==== List results ====");
    Serial.print("Querying with: ");
    Serial.println(query);
    Serial.println();

    FluxQueryResult result = client.query(query);

    while (result.next()) {
        String feature = result.getValueByName("feature").getString();
        Serial.print(feature);
        Serial.print(":  ");
        double value = result.getValueByName("_value").getDouble();
        Serial.print(value);

        FluxDateTime ACtime = result.getValueByName("_time").getDateTime();
        String timeStr = ACtime.format("%T");

        Serial.print(" at ");
        Serial.print(timeStr);
        Serial.println();

        if (feature == "AC-power") {
            ACpower = value;
        }
        if (feature == "AC-daily") {
            ACdaily = value;
        }
        if (feature == "AC-total") {
            ACtotal = value;
        }
    }
    Serial.println();

    if (result.getError().length() > 0) {
        Serial.print("Query result error: ");
        Serial.println(result.getError());
    }
    result.close();
}