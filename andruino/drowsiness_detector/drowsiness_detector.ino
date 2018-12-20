/*
  Optical Heart Rate Detection (PBA Algorithm) using the MAX30105 Breakout
  By: Nathan Seidle @ SparkFun Electronics
  Date: October 2nd, 2016
  https://github.com/sparkfun/MAX30105_Breakout

  This is a demo to show the reading of heart rate or beats per minute (BPM) using
  a Penpheral Beat Amplitude (PBA) algorithm.

  It is best to attach the sensor to your finger using a rubber band or other tightening
  device. Humans are generally bad at applying constant pressure to a thing. When you
  press your finger against the sensor it varies enough to cause the blood in your
  finger to flow differently which causes the sensor readings to go wonky.

  Hardware Connections (Breakoutboard to Arduino):
  -5V = 5V (3.3V is allowed)
  -GND = GND
  -SDA = A4 (or SDA)
  -SCL = A5 (or SCL)
  -INT = Not connected

  The MAX30105 Breakout can handle 5V or 3.3V I2C logic. We recommend powering the board with 5V
  but it will also run at 3.3V.
*/

#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <SPI.h>
#include <RF24.h>
#include <BTLE.h>
#include <math.h>

#define BUZZER_PIN 6
//Increase this for more averaging
#define RATE_SIZE 20 
#define RRI_SIZE 20
#define Interval 2000
#define DROWSYALERT 0.1

MAX30105 particleSensor;
RF24 radio(9,10);
BTLE btle(&radio);

float rates[RATE_SIZE]; //Array of heart rates
float rris[RRI_SIZE]; //Array of RR intervals, to calculate HRV
byte rateSpot = 0;
byte rriSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred
unsigned long logTimer = 0; //Time to print a log

float beatsPerMinute;
float hrv;
byte beatAvg;
byte drowsiness;

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
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  Serial.println("Initializing...");

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
  btle.begin("DrwsDtct");
}

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
      rris[rriSpot++] = delta / 1000.0; //rri in second
      
      rateSpot %= RATE_SIZE; //Wrap variable
      rriSpot %= RRI_SIZE; //Wrap variable
    }
  }
  if (rris[RRI_SIZE-1] && millis() - logTimer > Interval) 
  {
    beatAvg = (byte)average(rates, RATE_SIZE);
    hrv = rmssd(rris, RRI_SIZE);
    drowsiness = (byte)(min(255, 100 * DROWSYALERT / hrv));
    Serial.println();
    Serial.print("IR=");
    Serial.print(irValue);
    Serial.print(", BPM=");
    Serial.print(beatsPerMinute);
    Serial.print(", Avg BPM=");
    Serial.print(beatAvg);
    Serial.print(", HRV=");
    Serial.print(hrv);
    Serial.print(", Drowsiness=");
    Serial.print(drowsiness);
    
    nrf_service_data buf;
    buf.service_uuid = NRF_DEVICE_INFORMATION_SERVICE_UUID;
    buf.value = (drowsiness << 8) | beatAvg;  // send data x and y by (value = y << 8 | x)
    if(!btle.advertise(0x16, &buf, sizeof(buf))) {
      Serial.begin(115200);
      Serial.println("BTLE advertisement failure");
      Serial.end();
    }
    btle.hopChannel();
    if (irValue < 50000) 
    {
      digitalWrite(BUZZER_PIN, LOW); 
      Serial.print(" No finger?");
    }
    else if (hrv > 0 && hrv < DROWSYALERT) 
    {
      digitalWrite(BUZZER_PIN, HIGH);
      Serial.print("\nDrowsiness Detected");
    }
    else digitalWrite(BUZZER_PIN, LOW); 
    Serial.println();
    logTimer = millis();
    Serial.println();
  }
}
