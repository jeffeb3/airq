
#include "HMPA.h"

namespace hmpa
{
Sensor::Sensor(Stream* stream) :
    mpStream(stream),
    mHasMeasurement(false),
    mPm1(-1.0),
    mPm25(-1.0),
    mPm4(-1.0),
    mPm10(-1.0)
{
}

bool Sensor::hasMeasurement() const
{
    return mHasMeasurement;
}

float Sensor::getPM1() const
{
    if (not mHasMeasurement)
    {
        return -1;
    }
    return mPm1;
}

float Sensor::getPM25() const
{
    if (not mHasMeasurement)
    {
        return -1;
    }
    return mPm25;
}

float Sensor::getPM4() const
{
    if (not mHasMeasurement)
    {
        return -1;
    }
    return mPm4;
}

float Sensor::getPM10() const
{
    if (not mHasMeasurement)
    {
        return -1;
    }
    return mPm10;
}

// equation from
// https://www.airnow.gov/sites/default/files/2020-05/aqi-technical-assistance-document-sept2018.pdf
float Sensor::getAQI25() const
{
    float pm25 = getPM25();
    if (pm25 <   0.0) { return -1.0; }
    if (pm25 <  12.0) { return (pm25) / 12.0 * 50.0; }
    if (pm25 <  35.4) { return (pm25 -  12.0) * 50.0 / ( 35.4 - 12.0) +  50.0; }
    if (pm25 <  55.4) { return (pm25 -  35.4) * 50.0 / ( 55.4 - 35.4) + 100.0; }
    if (pm25 < 150.4) { return (pm25 -  55.4) * 50.0 / (150.4 - 55.4) + 150.0; }
    if (pm25 < 250.4) { return (pm25 - 150.4) + 200.0; }
    if (pm25 < 350.4) { return (pm25 - 250.4) + 300.0; }
    return (pm25 - 350.4) + 400.0;
}

// equation from
// https://www.airnow.gov/sites/default/files/2020-05/aqi-technical-assistance-document-sept2018.pdf
float Sensor::getAQI10() const
{
    float pm10 = getPM10();
    if (pm10 < 0.0)   { return -1.0; }
    if (pm10 < 54.0)  { return (pm10) / 54.0 * 50.0; }
    if (pm10 < 154.0) { return (pm10 -  54.0) *  50.0 / (154.0 - 54.0) + 50.0; }
    if (pm10 < 254.0) { return (pm10 - 154.0) *  50.0 / (254.0 - 154.0) + 100.0; }
    if (pm10 < 354.0) { return (pm10 - 254.0) *  50.0 / (354.0 - 254.0) + 150.0; }
    if (pm10 < 424.0) { return (pm10 - 354.0) * 100.0 / (424.0 - 354.0) + 200.0; }
    if (pm10 < 504.0) { return (pm10 - 424.0) * 100.0 / (504.0 - 424.0) + 300.0; }
    return (pm10 - 504.0) + 400.0;
}

// Short sighted helper to send 4  bytes.
bool sendCommand(Stream* stream, byte* command)
{
    stream->write(command, sizeof(command));

    const int bufferLength(2);
    byte buffer[bufferLength];
    // This will return after the timeout.
    int bytesRead = stream->readBytes(buffer, bufferLength);

    if (bytesRead < bufferLength)
    {
        // we timed out
        return false;
    }

    if (buffer[0] == 0xA5 and
        buffer[1] == 0xA5)
    {
        // ACK
        debugPrintln("[pm25]\tSend OK");
        return true;
    }

    // Whatever else we read, it's wrong.
    return false;
}

bool Sensor::startFan()
{
    // First, we send the command
    byte start_measurement[] = {0x68, 0x01, 0x01, 0x96 };
    bool isOk = sendCommand(mpStream, start_measurement);
    // TODO Should we clear stuff when we get an error?
    return isOk;
}

bool Sensor::stopFan()
{
    // First, we send the command
    byte stop_measurement[] = {0x68, 0x01, 0x02, 0x95 };
    bool isOk = sendCommand(mpStream, stop_measurement);
    // TODO Should we clear stuff when we get an error?
    return isOk;
}

bool Sensor::stopAutoSend()
{
    // First, we send the command
    byte stop_autosend[] = {0x68, 0x01, 0x20, 0x77 };
    bool isOk = sendCommand(mpStream, stop_autosend);
    // TODO Should we clear stuff when we get an error?
    return isOk;
}

byte calcChecksum(const byte head,
                  const int messageLength,
                  const byte* messageBuffer)
{
    int sum = 0;
    for (int index = 0; index < messageLength; index++)
    {
        sum += messageBuffer[index];
    }
    return (0x10000 - head - messageLength - sum) % 0XFF;
}

void clear(Stream* stream)
{
    // Clear out however many things are in there.
    debugPrintln(String("available: ") + String(stream->available()));
    int junkLength = std::min(64, stream->available());
    byte junk[junkLength];
    junkLength = stream->readBytes(junk, junkLength);
    for (int index = 0; index < junkLength; index++)
    {
        debugPrintln(String(" 0x") + String(junk[index], HEX));
    }
    stream->flush();
}

bool Sensor::read()
{
    byte read_particle[] = {0x68, 0x01, 0x04, 0x93 };
    clear(mpStream);
    mpStream->write(read_particle, sizeof(read_particle));

    // We need to read this message in pieces.
    byte header[2];
    // This will return after the timeout.
    int bytesRead = mpStream->readBytes(header, 2);
    if (bytesRead < 2 or
        header[0] != 0x40 or
        header[1] > 30)
    {
        // we timed out, or this isn't the message we are interested in.
        debugPrintln("[pm25]\tErr: Header");
        debugPrintln(String("[pm25]\t[") + String(bytesRead) + String("] 0x") + String(header[0],HEX) + String(" 0x") + String(header[1],HEX));
        mHasMeasurement = false;
        return false;
    }

    int messageLength = header[1];
    byte messageBuffer[messageLength];
    // This will return after the timeout.
    bytesRead = mpStream->readBytes(messageBuffer, messageLength);
    if (bytesRead < messageLength or messageBuffer[0] != 0x04)
    {
        debugPrintln("[pm25]\tErr: Bytes");
        // we timed out.
        mHasMeasurement = false;
        return false;
    }

    byte checksum;
    bytesRead = mpStream->readBytes(&checksum, 1);
    if (bytesRead < 1)
    {
        // we timed out
        debugPrintln("[pm25]\tErr: Checksum");
        mHasMeasurement = false;
        return false;
    }

    // we have a message and the checksum
    if (checksum != calcChecksum(header[0], messageLength, messageBuffer))
    {
        // We have a bad checksum.
        debugPrintln("[pm25]\tErr: Xsum");
        mHasMeasurement = false;
        return false;
    }

    // AFAICT, this data is good. Let's check it out.
    if (messageLength == 5)
    {
        // We have only pm25 and pm10
        mPm25 = 256.0 * messageBuffer[1] + messageBuffer[2];
        mPm10 = 256.0 * messageBuffer[3] + messageBuffer[4];
        mPm1 = -1.0;
        mPm4 = -1.0;
        mHasMeasurement = true;
    }
    else if (messageLength == 13)
    {
        for (int index = 0; index < messageLength; index++)
        {
            debugPrintln(String(" 0x") + String(messageBuffer[index], HEX));
        }

        // We have only pm25 and pm10
        mPm1  = 256.0 * messageBuffer[1] + messageBuffer[2];
        mPm25 = 256.0 * messageBuffer[3] + messageBuffer[4];
        debugPrintln(String("pm2.5: ") + mPm25);
        mPm4  = 256.0 * messageBuffer[5] + messageBuffer[6];
        mPm10 = 256.0 * messageBuffer[7] + messageBuffer[8];
        mHasMeasurement = true;
    }
    else
    {
        // Uh oh, what sensor is this?
        debugPrintln("[pm25]\tErr: Not right");
        mHasMeasurement = false;
        return false;
    }

    return true;
}

} // namespace
