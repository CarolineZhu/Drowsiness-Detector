# Drowsiness-Detector
A drowsiness detector iOS application for drivers based on heart rate with adruino pulse rate sensor, BLE and Microsoft Azure

we implemented an IoT system that can detect drowsiness when driving and alarm the driver if necessary. the basic idea is We use Arduino Uno Dev Board as microprocessor and Particle Sensor to get pulse rate of the driver, computed BPM and HRV according to it on dev board, they are two important arguments to determine drowsisness, and then evaluate a score to describe the drowsy degree of the driver and sent the data to the ios App through Bluetooth low energy, so the driver can see his evaluated score. Meanwhile we attached a bazzer so that it can alarm the driver if the score exceeds the defined threshold. First we need some benchmarks to calculate the specific value of threshold.We did this on Microsoft Azure with WiFi connection to our dev board in the beginning. It is intuitive that we can access the WiFi during the warmup stage, for example do this inside a building. However when we need to apply the drowsiness detection function on a person who is driving on a highway without internet environment, we can only use the BTLE to communicate, to send data between the sensor, microprocessor and the ios APP.
