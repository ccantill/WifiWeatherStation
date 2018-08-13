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

const int MAGIC_NUMBER = 0x4a414bc1;

struct settings {
  int magicNumber;
  bool deepSleepMode;
  bool influxEnabled;
  char influxHostName[20];
  unsigned short influxPort;
  char influxDatabase[20];
  char influxSeries[20];
  char influxTags[30];
};

// Include the correct display library
// For a connection via I2C using Wire include
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
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
SSD1306  display(0x3c, D2, D1);
ESP8266WebServer httpServer(80);

int screenW = 128;
int screenH = 64;

float temperature_C = 22.2;
float humidity_pct = 80;
String ip = "192.168.1.52";
boolean connected = true;

WiFiManager wifiManager;

void displaySetUpWifi(WiFiManager *wifiManager) {
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64,0, "Connect to:");
  display.drawString(64,32, wifiManager->getConfigPortalSSID());

  display.display();
}

void updateDisplay() {
  String temperature = String(temperature_C, 1) + "°";
  String humidity = String(humidity_pct, 0) + "%";
  display.clear();
  display.setFont(ArialMT_Plain_24);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 32, temperature);
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(128, 32, humidity);

  display.setFont(Dialog_plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  if(WiFi.isConnected()) {
    display.drawString(0, 0, WiFi.localIP().toString());
    display.drawXbm(120, 2, status_width, status_height, status_wifi_full_bits);
  } else {
    display.drawString(0, 0, "Not connected");
  }

  display.display();
}

void initEeprom() {
  int magicNumber;
  EEPROM.get(offsetof(settings, magicNumber), magicNumber);
  if(magicNumber != MAGIC_NUMBER) {
    settings newSettings;
    newSettings.deepSleepMode = false;
    newSettings.influxEnabled = false;
    EEPROM.put(0, newSettings);
  }
}

void http_root() {
  httpServer.send(200, "text/html", "<html><head><title>WiFiWeatherStation Setup</title></head><body><a href='/resetWifi'>Reset WiFi settings</a></body></html>");
}

void http_resetWifi() {
  httpServer.send(200, "text/plain", "OK. Restarting the sensor.");
  wifiManager.resetSettings();
  ESP.restart();
}

void setup() {
  Serial.begin(115200);
  initEeprom();
  bool deepSleep;
  EEPROM.get(offsetof(settings, deepSleepMode), deepSleep);
  if(deepSleep) {
    Serial.println("Deep sleep not yet implemented");
    // not yet implemented
  } else {
    Serial.println("Initing display");
    display.init();
    display.flipScreenVertically();
    display.setContrast(255);
    display.clear();

    // setup callback for displaying setup instructions
    wifiManager.setAPCallback(displaySetUpWifi);

    // update display to reflect current connection state
    updateDisplay();

    wifiManager.autoConnect();

    // setup http endpoints
    httpServer.on("/", http_root);
    httpServer.on("/resetWifi", http_resetWifi);
    httpServer.begin();

    updateDisplay();
  }
}

void loop() {
  httpServer.handleClient();
}