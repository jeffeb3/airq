
#pragma once

#include "Arduino.h"
#include <ESP32BackBone.h>

// Class to encapsulate connection to the Honeywell HMPA115S0 sensors.
//
namespace hmpa
{
class Sensor
{
public:
    /// You need to keep the stream object from going out of scope yourself.
    Sensor(Stream* stream);

    /// If any of these functions return something less than zero, it's because we haven't had a
    /// measurement yet.
    bool hasMeasurement() const;
    /// Get the float value of pm1 in ug/m3 (not available on all hardware).
    float getPM1() const;
    /// Get the float value of pm2.5 in ug/m3
    float getPM25() const;
    /// Get the float value of pm4 in ug/m3
    float getPM4() const;
    /// Get the float value of pm10 in ug/m3
    float getPM10() const;

    /// Get the float value of air quality index from pm2.5
    float getAQI25() const;
    /// Get the float value of air quality index from pm10
    float getAQI10() const;

    /// Send the start fan command.
    /// @returns true if it read something, otherwise, it returns false.
    bool startFan();

    /// Send the stop fan command.
    /// @returns true if it read something, otherwise, it returns false.
    bool stopFan();

    /// Send the stop autosend command.
    /// @returns true if it read something, otherwise, it returns false.
    bool stopAutoSend();

    /// Reads. May block.
    ///
    /// You can use setTimeout on the stream to make it not block.
    /// @returns true if it read something, otherwise, it returns false.
    bool read();

private:

    Stream* mpStream;

    bool mHasMeasurement;
    float mPm1;
    float mPm25;
    float mPm4;
    float mPm10;
};

} // namespace

