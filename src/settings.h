struct struct_settings
{
    int magicNumber;
    int deepSleepTimer;
    bool influxEnabled;
    char influxHostName[20];
    unsigned short influxPort;
    char influxDatabase[20];
    char influxSeries[20];
    char influxTags[30];
};

const int MAGIC_NUMBER = 0x1a512f57;

struct_settings settings;

void saveSettings()
{
    Serial.println("Storing settings");
    EEPROM.put(0, settings);
    EEPROM.commit();
}

void loadSettings()
{
    EEPROM.begin(sizeof settings);
    EEPROM.get(0, settings);
    Serial.println("Settings magic number is " + String(settings.magicNumber, 16));
    if (settings.magicNumber != MAGIC_NUMBER)
    {
        Serial.println("Resetting settings");
        settings.magicNumber = MAGIC_NUMBER;
        settings.deepSleepTimer = 10; // every 10 seconds
        settings.influxEnabled = false;
        saveSettings();
    }
}