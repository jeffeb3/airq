/**
 * Blink
 *
 * Turns on an LED on for one second,
 * then off for one second, repeatedly.
 */
#include "Arduino.h"
#include <ESP32BackBone.h>

#define LED_BUILTIN 2

String name = "airq";
int loop_count = 0;
int display_count = 0;

void setup()
{
    // "Required" before starting anything else in the backbone.
    espbb::setup();

    // Initialize the display.
    espbb::setDisplay(false);

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

            // Draw the loop count
            display.setTextAlignment(TEXT_ALIGN_RIGHT);
            display.setFont(ArialMT_Plain_10);
            display.drawString(128, 0, String(loop_count));

            // Draw the display count. Notice how this is updated in the display thread, so it's
            // much higher than the loop counter.
            display.setTextAlignment(TEXT_ALIGN_RIGHT);
            display.setFont(ArialMT_Plain_10);
            display.drawString(128, 12, String(display_count));

            // Say "Hello"
            display.setTextAlignment(TEXT_ALIGN_LEFT);
            display.setFont(ArialMT_Plain_24);
            display.drawString(0, 20, "Hello" );

            display_count += 1;
        });

    espbb::addPage(
        [](OLEDDisplay& display)
        {
            // Say "World"
            display.setTextAlignment(TEXT_ALIGN_LEFT);
            display.setFont(ArialMT_Plain_24);
            display.drawString(0, 20, "World" );
        });

    // Start the pages rotating every 7.5 seconds
    espbb::autoRotatePages(7.5);
    // For the Status LED
    pinMode(2,OUTPUT);
}

void loop()
{
    // I can't get this stupid thing to work in its own thread. Everything comes out all garbled.
    espbb::doDisplay();

    // Count the number of loops, to illustrate the way the display is multithreaded.
    loop_count += 1;

    //digitalWrite(2,HIGH);
    //debugPrintln("[MAIN]:\tOn");

    //vTaskDelay(1 * 1000 / portTICK_PERIOD_MS);

    //debugPrintln("[MAIN]:\tOff");
    //// This will slow down the loop(), This will sleep for 10 seconds, so the loop will run a little
    //// slower than 10 seconds.
    //digitalWrite(2,LOW);

    //vTaskDelay(1 * 1000 / portTICK_PERIOD_MS);
}


