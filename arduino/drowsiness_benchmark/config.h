// Physical device information for board and sensor
#define DEVICE_ID "ESP8266 Dev"

// Pin layout configuration
#define LED_PIN 0

#define DROWSY_ALERT 2

// Interval time(ms) for sending message to IoT Hub
#define INTERVAL 5000

//Increase this for more averaging
#define RATE_SIZE 20 
#define RRI_SIZE 20

// EEPROM address configuration
#define EEPROM_SIZE 512

// SSID and SSID password's length should < 32 bytes
// http://serverfault.com/a/45509
#define SSID_LEN 32
#define PASS_LEN 32
#define CONNECTION_STRING_LEN 256

#define MESSAGE_MAX_LEN 256
