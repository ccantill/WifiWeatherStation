/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 by ThingPulse, Daniel Eichhorn
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * ThingPulse invests considerable time and money to develop these open source libraries.
 * Please support us by buying our products (and not the clones) from
 * https://thingpulse.com
 *
 */

#include "font.h"
#include "images.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include "settings.h"
#include <ClosedCube_SHT31D.h>
#include <Ticker.h>

#include "influx.h"

// Include the correct display library
// For a connection via I2C using Wire include
#include <Wire.h>    // Only needed for Arduino 1.6.5 and earlier
#include <SSD1306.h> // alias for `#include "SSD1306Wire.h"`
// or #include "SH1106.h" alis for `#include "SH1106Wire.h"`
// For a connection via I2C using brzo_i2c (must be installed) include
// #include <brzo_i2c.h> // Only needed for Arduino 1.6.5 and earlier
// #include "SSD1306Brzo.h"
// #include "SH1106Brzo.h"
// For a connection via SPI include
// #include <SPI.h> // Only needed for Arduino 1.6.5 and earlier
// #include "SSD1306Spi.h"
// #include "SH1106SPi.h"

// Initialize the OLED display using brzo_i2c
// D3 -> SDA
// D5 -> SCL
// SSD1306Brzo display(0x3c, D3, D5);
// or
// SH1106Brzo  display(0x3c, D3, D5);

// Initialize the OLED display using Wire library
SSD1306 display(0x3c, D2, D1);
ESP8266WebServer httpServer(80);

const int WAKE_UP_PIN = 14;

const float temperature_threshold = 0.2;
const float humidity_threshold = 1;

int screenW = 128;
int screenH = 64;

bool inLowPowerMode = false;
bool syncNeeded = false;

struct STATE {
  float temperature_C;
  float humidity_pct;
};

STATE state = {
  NAN, NAN
};

WiFiManager wifiManager;
ClosedCube_SHT31D sht3xd;
Ticker ticker;

String SHT3XD_Error_to_String(SHT31D_ErrorCode err)
{
  switch (err)
  {
  case SHT3XD_NO_ERROR:
    return "SHT3XD_NO_ERROR";
  case SHT3XD_CRC_ERROR:
    return "SHT3XD_CRC_ERROR ";
  case SHT3XD_TIMEOUT_ERROR:
    return "SHT3XD_TIMEOUT_ERROR ";
  case SHT3XD_PARAM_WRONG_MODE:
    return "SHT3XD_PARAM_WRONG_MODE ";
  case SHT3XD_PARAM_WRONG_REPEATABILITY:
    return "SHT3XD_PARAM_WRONG_REPEATABILITY ";
  case SHT3XD_PARAM_WRONG_FREQUENCY:
    return "SHT3XD_PARAM_WRONG_FREQUENCY ";
  case SHT3XD_PARAM_WRONG_ALERT:
    return "SHT3XD_PARAM_WRONG_ALERT ";
  case SHT3XD_WIRE_I2C_DATA_TOO_LOG:
    return "SHT3XD_WIRE_I2C_DATA_TOO_LOG ";
  case SHT3XD_WIRE_I2C_RECEIVED_NACK_ON_ADDRESS:
    return "SHT3XD_WIRE_I2C_RECEIVED_NACK_ON_ADDRESS ";
  case SHT3XD_WIRE_I2C_RECEIVED_NACK_ON_DATA:
    return "SHT3XD_WIRE_I2C_RECEIVED_NACK_ON_DATA ";
  case SHT3XD_WIRE_I2C_UNKNOW_ERROR:
    return "SHT3XD_WIRE_I2C_UNKNOW_ERROR ";
  default: return "WTF?";
  }
}

void displaySetUpWifi(WiFiManager *wifiManager)
{
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 4, "Connect to:");
  display.drawString(64, 32, wifiManager->getConfigPortalSSID());

  display.display();
}

void updateDisplay()
{
  String temperature = String(state.temperature_C, 1) + "°";
  String humidity = String(state.humidity_pct, 0) + "%";
  display.clear();
  display.setFont(ArialMT_Plain_24);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 32, temperature);
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(128, 32, humidity);

  display.setFont(Dialog_plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  if (inLowPowerMode)
  {
    display.drawString(0, 0, "Press button to connect");
  }
  else if (WiFi.isConnected())
  {
    display.drawString(0, 0, WiFi.localIP().toString());
    display.drawXbm(120, 4, status_width, status_height, status_wifi_full_bits);
  }
  else
  {
    display.drawString(0, 0, "Not connected");
  }

  display.display();
}

void enterDeepSleep()
{
  ESP.rtcUserMemoryWrite(0, (uint32_t*) &state, sizeof(state));
  Serial.println("Sleeping for " + String(settings.deepSleepTimer) + " seconds");
  ESP.deepSleep(1e6 * settings.deepSleepTimer);
}

void http_root()
{
  httpServer.send(200, "text/html", "<html><head><title>WiFiWeatherStation Setup</title></head><body><a href='/resetWifi'>Reset WiFi settings</a></body></html>");
}

void http_lowPower()
{
  Serial.println("Entering low power mode");
  httpServer.send(200, "text/plain", "OK. Entering low power mode");
  display.setContrast(settings.lowPowerContrast);
  inLowPowerMode = true;
  updateDisplay();
  enterDeepSleep();
}

void http_handleSettings() {
    StaticJsonBuffer<512> jsonBuffer;
    if(httpServer.method() == HTTP_GET) {
        JsonObject& root = jsonBuffer.createObject(); 
        JsonObject& influx = root.createNestedObject("influx");
        influx["enabled"] = settings.influxEnabled;
        influx["host"] = settings.influxHost;
        influx["port"] = settings.influxPort;
        influx["database"] = settings.influxDatabase;
        influx["series"] = settings.influxSeries;
        influx["tags"] = settings.influxTags;
        
        JsonObject& lowPower = root.createNestedObject("lowpower");
        lowPower["updateInterval"] = settings.deepSleepTimer;
        lowPower["contrast"] = settings.lowPowerContrast;

        JsonObject& general = root.createNestedObject("general");
        general["contrast"] = settings.displayContrast;

        String json;
        root.printTo(json);
        httpServer.send(200, "text/json", json);
    } else if(httpServer.method() == HTTP_POST) {
        JsonObject& root = jsonBuffer.parseObject(httpServer.arg("plain"));
        if(!root.success()) {
          httpServer.send(400, "text/plain", "Body could not be parsed");
        }
        Serial.println(httpServer.arg("plain"));

        int deepSleepTimer = root["lowpower"]["updateInterval"];
        bool influxEnabled = root["influx"]["enabled"];
        String influxDatabase = root["influx"]["database"];
        String influxHost = root["influx"]["host"];
        unsigned short influxPort = root["influx"]["port"];
        String influxSeries = root["influx"]["series"];
        String influxTags = root["influx"]["tags"];
        unsigned char contrast = root["general"]["contrast"];
        unsigned char lowPowerContrast = root["lowpower"]["contrast"];

        // validation
        if(influxEnabled && (
            influxDatabase.length() == 0 ||
            influxHost.length() == 0 ||
            influxSeries.length() == 0 || 
            influxPort == 0
        )) {
            httpServer.send(400, "text/plain", "Influx enabled but not enough details provided");
            return;
        }

        settings.deepSleepTimer = deepSleepTimer;
        settings.influxEnabled = influxEnabled;
        influxDatabase.getBytes((unsigned char*) &settings.influxDatabase, sizeof settings.influxDatabase, 0);
        influxPort = influxPort;
        influxHost.getBytes((unsigned char*) &settings.influxHost, sizeof settings.influxHost, 0);
        influxSeries.getBytes((unsigned char*) &settings.influxSeries, sizeof settings.influxSeries, 0);
        influxTags.getBytes((unsigned char*) &settings.influxTags, sizeof settings.influxTags, 0);
        settings.displayContrast = contrast;
        settings.lowPowerContrast = lowPowerContrast;
        saveSettings();

        display.setContrast(settings.displayContrast);

        httpServer.send(200, "text/plain", "Settings saved");
    } else {
        httpServer.send(405, "text/plain", "Method not allowed");
    }
}

void http_factoryReset() {
  httpServer.send(200, "text/plain", "Resetting to factory settings and restarting");
  resetSettings();
  wifiManager.resetSettings();
  ESP.restart();
}

void http_influxLastResponse() {
  httpServer.send(200, "text/plain", String(lastInfluxPostResult));
}

bool readClimate() {
  SHT31D data = sht3xd.periodicFetchData();
  if(data.error != SHT3XD_NO_ERROR) {
    Serial.println("[SHT3XD] Read error " + SHT3XD_Error_to_String(data.error));
    return false;
  }
  Serial.println("read " + String(data.t) + " and " + String(data.rh) + ". Previous readings were " + String(state.humidity_pct) + " and " + String(state.temperature_C));
  if(isnan(state.temperature_C) || isnan(state.humidity_pct) || fabs(data.t - state.temperature_C) > temperature_threshold || fabs(data.rh - state.humidity_pct) > humidity_threshold) {
    state.temperature_C = data.t;
    state.humidity_pct = data.rh;
    return true;
  }
  return false;
}

void sendUpdate()
{
  syncInflux(state.temperature_C, state.humidity_pct);
}

void updateClimate() {
  if(readClimate()) {
    syncNeeded = true;
  }
}

void setup()
{
  Wire.begin();
  Serial.begin(115200);
  loadSettings();
  pinMode(WAKE_UP_PIN, INPUT);
  
  rst_info* resetInfo = ESP.getResetInfoPtr();
  Serial.println("Reset reason " + String(resetInfo->reason, 16));

  if(sht3xd.begin(0x44)) {
    Serial.println("SHT31 failed to initialize");
  }

  if (sht3xd.periodicStart(SHT3XD_REPEATABILITY_HIGH, SHT3XD_FREQUENCY_10HZ) != SHT3XD_NO_ERROR) {
		Serial.println("[ERROR] Cannot start periodic mode");
  }

  int wakeUp = digitalRead(WAKE_UP_PIN);

  if((resetInfo->reason == REASON_DEEP_SLEEP_AWAKE)) {
    ESP.rtcUserMemoryRead(0, (uint32_t*) &state, sizeof(state));
  }

  if ((resetInfo->reason == REASON_DEEP_SLEEP_AWAKE) && !wakeUp)
  {
    Serial.println("Waking up from deep sleep!");
    // since we have no readings we're assuming they're always the same anyway
    bool changed = readClimate();
    if (changed)
    {
      inLowPowerMode = false;
      display.resume();
      updateDisplay();
      wifiManager.autoConnect();
      updateDisplay();

      sendUpdate();

      inLowPowerMode = true;
      updateDisplay();
    }
    enterDeepSleep();
  }
  else
  {
    inLowPowerMode = false;
    Serial.println("Initing display");

    if(resetInfo->reason == REASON_DEEP_SLEEP_AWAKE) {
      // when waking up from deep sleep, assume display still has its settings so don't init display, just init the library
      display.resume();
    } else {
      display.init();
      display.flipScreenVertically();
      display.setContrast(settings.displayContrast);
    }

    readClimate();

    // setup callback for displaying setup instructions
    wifiManager.setAPCallback(displaySetUpWifi);

    // update display to reflect current connection state
    updateDisplay();

    wifiManager.autoConnect();

    updateDisplay();

    // setup http endpoints
    httpServer.on("/", http_root);
    httpServer.on("/factoryReset", http_factoryReset);
    httpServer.on("/lowPower", http_lowPower);
    httpServer.on("/settings", http_handleSettings);
    httpServer.on("/influx/lastResponse", http_influxLastResponse);
    httpServer.begin();

    ticker.attach(1, updateClimate);
  }
}

void loop()
{
  httpServer.handleClient();
  if(syncNeeded) {
    syncNeeded = false;
    updateDisplay();
    sendUpdate();
  }
}