#ifndef __SETTINGS__
#define __SETTINGS__

#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

struct struct_settings
{
    int magicNumber;
    int deepSleepTimer;
    bool influxEnabled;
    char influxHost[20];
    unsigned short influxPort;
    char influxDatabase[20];
    char influxSeries[20];
    char influxTags[30];
};

const int MAGIC_NUMBER = 0x1a512f59;

struct_settings settings;

void saveSettings()
{
    Serial.println("Storing settings");
    EEPROM.put(0, settings);
    EEPROM.commit();
}

void resetSettings() {
    Serial.println("Resetting settings");
    settings.magicNumber = MAGIC_NUMBER;
    settings.deepSleepTimer = 10; // every 10 seconds
    settings.influxEnabled = false;
    settings.influxHost[0] = 0;
    settings.influxPort = 8086;
    String("climate").getBytes((unsigned char*) &(settings.influxSeries), sizeof settings.influxSeries, 0);
    String("name=Sensor 1").getBytes((unsigned char*) &(settings.influxTags), sizeof settings.influxSeries, 0);
    saveSettings();
}

void loadSettings()
{
    EEPROM.begin(sizeof settings);
    EEPROM.get(0, settings);
    Serial.println("Settings magic number is " + String(settings.magicNumber, 16));
    if (settings.magicNumber != MAGIC_NUMBER)
    {
        resetSettings();
    }
}
#endif
