// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Please use an Arduino IDE 1.6.8 or greater

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>

#include <AzureIoTHub.h>
#include <AzureIoTProtocol_MQTT.h>
#include <AzureIoTUtility.h>
#include <Wire.h>
#include "config.h"
#include "MAX30105.h"
#include "heartRate.h"
MAX30105 particleSensor;

static bool messagePending = false;
static bool messageSending = true;

static char *connectionString;
static char *ssid;
static char *pass;

static int interval = INTERVAL;
float rates[RATE_SIZE]; //Array of heart rates
float rris[RRI_SIZE]; //Array of RR intervals, to calculate HRV
byte rateSpot = 0;
byte rriSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred
unsigned long logTimer = 0; //Time to print a log

float beatsPerMinute;
byte beatAvg;
float hrv;

void blinkLED()
{
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
}

void initWifi()
{
    // Attempt to connect to Wifi network:
    Serial.printf("Attempting to connect to SSID: %s.\r\n", ssid);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED)
    {
        // Get Mac Address and show it.
        // WiFi.macAddress(mac) save the mac address into a six length array, but the endian may be different. The huzzah board should
        // start from mac[0] to mac[5], but some other kinds of board run in the oppsite direction.
        uint8_t mac[6];
        WiFi.macAddress(mac);
        Serial.printf("You device with MAC address %02x:%02x:%02x:%02x:%02x:%02x connects to %s failed! Waiting 10 seconds to retry.\r\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], ssid);
        WiFi.begin(ssid, pass);
        delay(10000);
    }
    Serial.printf("Connected to wifi %s.\r\n", ssid);
}

void initTime()
{
    time_t epochTime;
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    while (true)
    {
        epochTime = time(NULL);

        if (epochTime == 0)
        {
            Serial.println("Fetching NTP epoch time failed! Waiting 2 seconds to retry.");
            delay(2000);
        }
        else
        {
            Serial.printf("Fetched NTP epoch time is: %lu.\r\n", epochTime);
            break;
        }
    }
}

static IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;

float average(float *nums, byte sz) 
{
  float avg = 0;
  for (byte i = 0 ; i < sz ; i++) avg += nums[i];
  return avg / sz;
}

float sdnn(float *nums, byte sz) 
{
  float sd = 0;
  float avg = average(nums, sz);
  for (byte i = 0 ; i < sz ; i++) sd += sq(nums[i] - avg);
  return sqrt(sd / (sz - 1));
}

float rmssd(float *nums, byte sz) 
{
  float rms = 0;
  for (byte i = 1 ; i < sz ; i++) rms += sq(nums[i] - nums[i-1]);
  return sqrt(rms / (sz - 1));
}

void setup()
{
    pinMode(LED_PIN, OUTPUT);
    initSerial();
    delay(2000);
    readCredentials();

    initWifi();
    initTime();

    /*
     * AzureIotHub library remove AzureIoTHubClient class in 1.0.34, so we remove the code below to avoid
     *    compile error
    */

    // initIoThubClient();
    iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, MQTT_Protocol);
    if (iotHubClientHandle == NULL)
    {
        Serial.println("Failed on IoTHubClient_CreateFromConnectionString.");
        while (1);
    }

    IoTHubClient_LL_SetOption(iotHubClientHandle, "product_info", "HappyPath_AdafruitFeatherHuzzah-C");
    IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, receiveMessageCallback, NULL);
    IoTHubClient_LL_SetDeviceMethodCallback(iotHubClientHandle, deviceMethodCallback, NULL);
    IoTHubClient_LL_SetDeviceTwinCallback(iotHubClientHandle, twinCallback, NULL);
    Serial.println("MAX30105 Initializing...");
  
    // Initialize sensor
    if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
    {
        Serial.println("MAX30105 was not found. Please check wiring/power. ");
        while (1);
    }
    Serial.println("Place your index finger on the sensor with steady pressure.");
    
    particleSensor.setup(); //Configure sensor with default settings
    particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
    particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
    Serial.println("Start collecting data...Warm up..."); // fill up rri array, around 2 minutes

    
}

static int messageCount = 1;
void loop()
{
    long irValue = particleSensor.getIR();
    if (checkForBeat(irValue) == true)
    {
        //We sensed a beat!
        long delta = millis() - lastBeat;
        lastBeat = millis();
  
        beatsPerMinute = 60 / (delta / 1000.0);
  
        if (beatsPerMinute > 20 && beatsPerMinute < 255)
        {
            rates[rateSpot++] = beatsPerMinute; //Store this reading in the array
            rris[rriSpot++] = delta / 1000.0; //rri in seconds
            
            rateSpot %= RATE_SIZE; //Wrap variable
            rriSpot %= RRI_SIZE; //Wrap variable
        }
    }
    if (rris[RRI_SIZE-1] && millis() - logTimer > INTERVAL && !messagePending && messageSending) 
    {
        beatAvg = (byte)average(rates, RATE_SIZE);
        hrv = rmssd(rris, RRI_SIZE);
        char messagePayload[MESSAGE_MAX_LEN];
        bool drowsy = readMessage(beatAvg, hrv, messageCount, messagePayload);
        sendMessage(iotHubClientHandle, messagePayload, drowsy);
        messageCount++;
        delay(interval);
        logTimer = millis();
    }
    IoTHubClient_LL_DoWork(iotHubClientHandle);
    delay(10);
}
