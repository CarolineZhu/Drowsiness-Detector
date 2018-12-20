#include <ArduinoJson.h>

bool readMessage(byte bpm, float hrv, int messageId, char *payload)
{
    StaticJsonBuffer<MESSAGE_MAX_LEN> jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root["deviceId"] = DEVICE_ID;
    root["messageId"] = messageId;
    bool drowsy = false;

    // NAN is not the valid json, change it to NULL
    if (std::isnan(bpm))
    {
        root["bpm"] = NULL;
    }
    else
    {
        root["bpm"] = bpm;
    }

    if (std::isnan(hrv))
    {
        root["hrv"] = NULL;
    }
    else
    {
        root["hrv"] = hrv;
        if (hrv < DROWSY_ALERT)
        {
            drowsy = true;
        }
    }
    root.printTo(payload, MESSAGE_MAX_LEN);
    return drowsy;
}

void parseTwinMessage(char *message)
{
    StaticJsonBuffer<MESSAGE_MAX_LEN> jsonBuffer;
    JsonObject &root = jsonBuffer.parseObject(message);
    if (!root.success())
    {
        Serial.printf("Parse %s failed.\r\n", message);
        return;
    }

    if (root["desired"]["interval"].success())
    {
        interval = root["desired"]["interval"];
    }
    else if (root.containsKey("interval"))
    {
        interval = root["interval"];
    }
}
