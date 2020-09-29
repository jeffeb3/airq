//
// Airq
// An air quality monitor project for my IoT
//

#include "HMPA.h"

#include "Arduino.h"
#include <ESP32BackBone.h>
#include <SoftwareSerial.h>

#define LED_BUILTIN 2

String name = AIRQ_NAME;
unsigned long lastTime = 0;
bool awake = false;
// Awake for 15s every 5 mins.
// awake less than MAX_AWAKE_TIME is on.
const unsigned long MAX_AWAKE_TIME = 1000 * 60 * 60 * 24 * 30;
// awake less than MAX_AWAKE_CYCLE_TIME is off
const unsigned long MAX_AWAKE_CYCLE_TIME = 1000 * 60 * 15;
long awakeTime = -MAX_AWAKE_CYCLE_TIME;

#ifdef BBDEBUG
SoftwareSerial swSerial(22, 21); // RX, TX
hmpa::Sensor sensor(&swSerial);
#else
hmpa::Sensor sensor(&Serial);
#endif

void setup()
{
    constexpr long SERIAL_TIMEOUT_MS = 100;
    #ifdef BBDEBUG
    swSerial.begin(9600);
    swSerial.setTimeout(SERIAL_TIMEOUT_MS);
    #else
    Serial.begin(9600);
    Serial.setTimeout(SERIAL_TIMEOUT_MS);
    #endif

    // "Required" before starting anything else in the backbone.
    espbb::setup();

    // Configure the wifi using build flags (or you can replace these flags with your wifi info, as
    // strings like "ssid", "secret".
    espbb::setWiFi(MY_SSID, MY_WIFI_PASSWORD);

    // Configure OTA.
    espbb::setOta(MY_OTA_PASSWORD, name.c_str());

    // Configure Mqtt
    espbb::setMqtt(IPAddress(MY_MQTT_ADDRESS), name);

    // Initialize the display.
    constexpr bool startThread=false;
    constexpr bool verticalFlip=false;
    espbb::setDisplay(startThread, verticalFlip);

    // Add two "pages" to the display. One says "Hello" and one says "World".
    //
    // Page 1 also shows how to use global data on the page.
    //
    espbb::addPage(
        [](OLEDDisplay& display)
        {
            // Draw the name
            display.setTextAlignment(TEXT_ALIGN_LEFT);
            display.setFont(ArialMT_Plain_10);
            display.drawString(0, 0, name);

            if (sensor.hasMeasurement())
            {
                // Draw the pm2.5
                int progress_pm25 = std::min(100, std::max(0, int(100.0*sensor.getPM25()/200.0))); // out of 200
                display.setFont(ArialMT_Plain_10);
                display.drawString(0, 16, String("PM2.5 ") + String(sensor.getPM25(),1));
                display.drawProgressBar(67, 16, 60, 10, progress_pm25);

                // Draw the pm10
                int progress_pm10 = std::min(100, std::max(0, int(100.0*sensor.getPM10()/200.0))); // out of 200
                display.setFont(ArialMT_Plain_10);
                display.drawString(0, 32, String("PM10 ") + String(sensor.getPM10(),1));
                display.drawProgressBar(67, 32, 60, 10, progress_pm10);
            }
            else
            {
                display.setFont(ArialMT_Plain_16);
                display.drawString(0, 32, String("No Data"));
            }
        });

    // For the Status LED
    pinMode(LED_BUILTIN,OUTPUT);

    // Turn on/off the light with:
    //    mosquitto_pub -h ip_addr -t /led -m "1"
    //    mosquitto_pub -h ip_addr -t /led -m "0"
    espbb::subscribe(String("/led"),
        [](byte* payload, unsigned int length)
        {
            debugPrintln("[MQTT]:\tRx");
            bool pinState = digitalRead(LED_BUILTIN);
            digitalWrite(LED_BUILTIN, !pinState);
        });

    sensor.stopFan();
    sensor.stopAutoSend();
}

void takeMeasurement()
{
    sensor.read();
    espbb::publish(String("/evilhouse/") + name + String("/pm1"), String(sensor.getPM1()));
    espbb::publish(String("/evilhouse/") + name + String("/pm2.5"), String(sensor.getPM25()));
    espbb::publish(String("/evilhouse/") + name + String("/pm4"), String(sensor.getPM4()));
    espbb::publish(String("/evilhouse/") + name + String("/pm10"), String(sensor.getPM10()));
    espbb::publish(String("/evilhouse/") + name + String("/aqi2.5"), String(sensor.getAQI25()));
    espbb::publish(String("/evilhouse/") + name + String("/aqi10"), String(sensor.getAQI10()));
}


void loop()
{
    // I can't get this stupid thing to work in its own thread. Everything comes out all garbled.
    espbb::doDisplay();

    if (millis() - awakeTime > MAX_AWAKE_CYCLE_TIME)
    {
        if (not awake)
        {
            debugPrintln("[main]:\tAwake");
            // wake up
            awake = sensor.startFan();
            digitalWrite(LED_BUILTIN, awake);
        }

        if (awake)
        {
            awakeTime = millis();
        }
    }
    else if (millis() - awakeTime > MAX_AWAKE_TIME)
    {
        if (awake)
        {
            debugPrintln("[main]:\tAsleep");
            // go to sleep
            if (not sensor.stopFan())
            {
                // We couldn't stop the fan. But we shouldn't be awake anyway.
                debugPrintln("[main]\tError stopping fan");
            }
            awake = false;
            digitalWrite(LED_BUILTIN, awake);
        }
    }

    // Don't measure if we aren't awake.
    // Don't measure if you just did.
    // Don't measure if we haven't been awake for long
    if (awake and
        millis() - lastTime > 30 * 1000 and
        millis() - awakeTime > 10 * 1000)
    {
        debugPrintln("[main]:\tMeasure");
        lastTime = millis();
        takeMeasurement();
    }
}


