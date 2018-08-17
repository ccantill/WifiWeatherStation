#ifndef __INFLUX__
#define __INFLUX__

#include "settings.h"
#include <Arduino.h>
#include <ESP8266HTTPClient.h>

int lastInfluxPostResult = 0;

void syncInflux(float temperature_C, float humidity_pct) {
    if(settings.influxEnabled && WiFi.isConnected()) {
        Serial.println("Syncing to influx");
        String url = "/write?db=" + String(settings.influxDatabase);
        String payload = String(settings.influxSeries);
        if(settings.influxTags[0] != 0) {
            payload += "," + String(settings.influxTags);
        }
        payload += " temperature_C=" + String(temperature_C, 1) + ",humidity=" + String(humidity_pct, 0);
        
        HTTPClient http;
        http.begin(settings.influxHost, settings.influxPort, url);
        Serial.println("Sending request to " + String(settings.influxHost) + ":" + String(settings.influxPort) + " / " + url + " for: " + payload);
        lastInfluxPostResult = http.POST(payload);
        if(lastInfluxPostResult == HTTPC_ERROR_CONNECTION_REFUSED) {
            Serial.println("Influx refused connection");
        } else {
            Serial.println("Influx replied " + String(lastInfluxPostResult));
        }

        http.end();
    }
}

#endif
