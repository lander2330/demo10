// Sketch: demo10.ino
// Version: 2026.10.19
// Author: Larry Anderson

// This ESP32 microcontroller project shows how to perform several useful tasks including:

// 1. Uses Espressif Systems ESP32-C6-DEVKITC-1-N8 microcontroller with 8MB flash memory and onboard RGB LED.
// 2. Displays text and bitmaps on SSD1306 OLED to display.
// 3. Reads Infrared receiver (IR) key codes sent from an alphanumeric keyboard remote control.
// 4. Displays and navigates hierarchical menu system on OLED display to change configuration settings. 
// 5. Writes and reads WiFi ssid and password settings to a microsd memory card.
// 6. Uses those permanently stored settings to logon to an existing WiFi network.
// 7. Reads BME280 sensor to retrieve current ambient air temperature, humidity, and altitude readings.
// 8. Uses the BME280 temperature sensor readings to act as a thermostat to toggle a relay switch on and off.  
// 9. Sets and reads DS3231 real time clock to name files and timestamp temperature data.
//10. Writes the historical temperature and humidity information to a standard .csv file on the microsd.
//11. Sorts microsd filenames in descending order most recent to oldest.
//12. Hosts a web server that sends current and historical temperature data to cell phones, tablets, laptops, etc. 
//13. Downloads microsd .csv file from web server to web browser on cell phone, tablet, laptop or desktop.
//14. Multicast DNS navigation to http://mywebserver.local opposed to an ip address http://192...
//15. Uses xml protocol between web browser and web server to efficiently exchange data and remote commands.
//16. Uses cell phone as remote control to select, display, download, and delete temperature history reports.
//17. Sketch demonstrates c++, html, css, javascript, and document object model (DOM) coding techniques.
//18. Web browser webpage javascript supports receiving and processing unsolicited messages from the web server.
//19. Displays QR code image on OLED display used by cell phone camera to conveniently connect browser to web server.
//20. Changes ESP32 microcontroller onboard RGB color LED to convey information and error diagnostics.
//21. Supports over-the-air WiFi updates to replace its ESP32 firmware stored sketch.

// This sketch contains coding statements used by a variety of different 
// software languages, each with their own rules for entering ignored comments. 
//
// The c++ language uses // for comments as well as
// starting comments with /** and ending them with */.
// HTML starts comments with <!-- and ends them with -->
// CSS starts comments with /* and ends them with */
// CSS can also starts comments with /** and end them with */
// JavaScript also starts comments with /* and ends them with */
// JavaScript can also start comments with /** and end with */ 
// JavaScript like c++ can also use // for comments.

// The large memory and hardware requirements of this sketch requires the
// Espressif Systems ESP32-C6-DEVKITC-1-N8 microcontroller
// because it includes the larger onboard 8MB flash memory
// as well as an onboard RGB color changeable LED. 

// on Amazon Espressif Systems ESP32-C6-DEVKITC-1-N8 Microcontroller
// Arduino IDE -> Tools -> Board: ESP32C6 Dev Module
// Arduino IDE -> Tools -> Flash Size: 8MB(64Mb)
// Arduino IDE -> Tools -> Partition Scheme: 8M with spiffs (3MB APP/1.5MB SPIFFS)

// standard arduino api library
#include <Arduino.h>

// vector library 
#include <vector>

// library that supports sorting the vector
#include <algorithm>

// WiFi and web server libraries
#include <WiFi.h>
#include <HTTPClient.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
AsyncWebServer server(80);
IPAddress serverIp; 
char serverIpDisplay[48];

// event source url
AsyncEventSource events("/events");

// WiFi over-the-air update library
#include <ESPAsyncHTTPUpdateServer.h>
ESPAsyncHTTPUpdateServer updateServer;

// I2C wire library
#include <Wire.h>

// Espressif Systems ESP32-C6-DEVKITC-1-N8 Microcontroller uses these pins
// for I2C protocol to the BME280, SSD1306 oled, and DS3231 real time clock
#define SDA_PIN 21
#define SCL_PIN 22

// on amazon UCTRONICS 0.96 Inch OLED Module 12864
// SSD1306 oled display library
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define OLED_WIDTH 128 // OLED display width, in pixels
#define OLED_HEIGHT 64 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

int displayCycle = 0;
char displayArea[24];

int confirmMenuIndex = 1;
int mainMenuIndex = 1;

// on amazon DS3231SN real time clock
// DS3231 realtime clock library
#include "RTClib.h"
RTC_DS3231 rtc;

// on amazon BME280 Digital 5V Temperature Humidity Sensor
// BME280 library
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#define SEALEVELPRESSURE_HPA (1012.0)
Adafruit_BME280 bme;

// on amazon Micro SD SDHC TF Card Adapter Reader
// microsd related libraries
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// Espressif Systems ESP32-C6-DEVKITC-1-N8 Microcontroller uses these pins
// for SPI protocol to microsd memory card reader/writer
#define CS 4
#define SCK 5
#define MOSI 6
#define MISO 7

// variables control Espressif Systems ESP32-C6-DEVKITC-1-N8 Microcontroller pin wired to an optional relay switch
#define RELAY_PIN 9

// variables control Espressif Systems ESP32-C6-DEVKITC-1-N8 Microcontroller onboard RGB LED color
#include <Adafruit_NeoPixel.h>
constexpr uint8_t LED_PIN = 8;
constexpr uint8_t NUM_LEDS = 1;
Adafruit_NeoPixel rgbLed(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
struct RGB
    {
    uint8_t r, g, b;    
    };
constexpr RGB RGB_OFF = {0, 0, 0};
constexpr RGB RGB_PURPLE = {128, 0, 128};
constexpr RGB RGB_BLUE = {0, 0, 255};
constexpr RGB RGB_CYAN = {0, 255, 255};
constexpr RGB RGB_GREEN = {0, 255, 0};
constexpr RGB RGB_YELLOW = {255, 255, 0};
constexpr RGB RGB_MAGENTA = {255, 0, 255};
constexpr RGB RGB_RED = {255, 0, 0};
constexpr RGB RGB_WHITE = {255, 255, 255};

// the first time your ESP32 powers on with this sketch, these setting files are created on your microsd memory card.
// when necessary, edit these setting files on the microsd memory card to change the values used when your ESP32 is subsequently powered on.

char relayModeFilepath[] = "/relayMode.txt";
char relayModeDefault[] = "1";
char relayMode[2+1];
char relayState[12+1] = "Relay is Off";

char wifiSSIDFilepath[] = "/wifiSSID.txt";
char wifiSSIDDefault[] = "yourSSID";
char wifiSSID[32+1];

char wifiPasswordFilepath[] = "/wifiPassword.txt";
char wifiPasswordDefault[] = "yourPASSWORD";
char wifiPassword[32+1];

// global variables accessed repeatedly by one or more functions
DateTime datetime;
bool dataChanged = false;
float dataFloat;
char workarea[256];
char messageWorkarea[256];

String string = "";
String stringFilename = "";

bool refreshRequested = false;
char webpageFilepath[] = "/index.html";
char webpageWorkarea[30000] = "";
char webpage[30000] = "";

#define ITEMS_SIZE 30000
char xmlItem[ITEMS_SIZE];
char xmlItems[ITEMS_SIZE] = "";  

File file;
char reportFilepath[15+1] = "/ccyy-mm-dd.csv";
char reportFilename[14+1] = "ccyy-mm-dd.csv";
char reportData[10000] = "";
char reportFilenames[10000] = "";

char csvFilepath[] = "/ccyy-mm-dd.csv";
char csvFilename[] = "ccyy-mm-dd-csv";
int csvCount = 0;
char csvRecord[80];

#define LOW_THRESHOLD 999
#define HIGH_THRESHOLD -999

int rawTemperature;
int temperature;
int temperatureCalibration = 0;
int temperatureLow = LOW_THRESHOLD;
int temperatureHigh = HIGH_THRESHOLD;
int temperaturePrevious = 0;
char temperatureChar[8] = "0";
char calibrateTemperatureFilepath[] = "/calibrateTemperature.txt";
char calibrateTemperatureDefault[] = "0";
char calibrateTemperature[3+1];

int rawHumidity;
int humidity;
int humidityCalibration = 0;
int humidityLow = LOW_THRESHOLD;
int humidityHigh = HIGH_THRESHOLD;
int humidityPrevious = 0;
char humidityChar[8] = "0";
char calibrateHumidityFilepath[] = "/calibrateHumidity.txt";
char calibrateHumidityDefault[] = "0";
char calibrateHumidity[3+1];

int altitude;
char altitudeChar[8] = "0";

// remote control infrared led receiver
#include <IRremote.hpp>
const byte IR_RECEIVE_PIN = 10;

int irCodes[256];
int irKey;
int irCode;

// keys that are not on front or back sides of the dual side alphanumeric remote control
//const int KEYMISSINGCARET = 0x00; // 94
//const int KEYMISSINGSINGLEBACKQUOTE = 0x00; // 96
//const int KEYMISSINGLEFTBRACE = 0x00; // 123
//const int KEYMISSINGORBAR = 0x00; // 124
//const int KEYMISSINGRIGHTBRACE = 0x00; // 125

// keys on front of the dual side alphanumeric remote control
// these are transmitted by the infrared remote control 
const int KEYINPUT = 0x2F;
const int KEYPOWER = 0x08;
const int KEYAMAZON = 0xEA;
const int KEYNETFLIX = 0xEB;
const int KEYMGO = 0xED;
const int KEYBACKWARD = 0x35;
const int KEYPAUSE = 0x37;
const int KEYPLAY = 0x33;
const int KEYFORWARD = 0x36;
const int KEYCC = 0x39;
const int KEYRECORD = 0x34;
const int KEYSTOP = 0x30;
const int KEYINFO = 0x1B;
const int KEYEXIT = 0x49;
const int KEYMENU = 0x4F;
const int KEYOK = 0x44;
const int KEYUP = 0x45;
const int KEYDOWN = 0x46;
const int KEYLEFT = 0x47;
const int KEYRIGHT = 0x48;
const int KEYBACK = 0x4A;
const int KEYGUIDE = 0x1C;
const int KEYYELLOW = 0x52;
const int KEYBLUE = 0x53;
const int KEYRED = 0x54;
const int KEYGREEN = 0x55;
const int KEYVOLUMEUP = 0x02;
const int KEYVOLUMEDOWN = 0x03;
const int KEYMUTE = 0x09;
const int KEYVISIO = 0x2D;
const int KEYREPLAY = 0x1A;
const int KEYCHANNELUP = 0x00;
const int KEYCHANNELDOWN = 0x01;
const int KEY0 = 0x10; // 48
const int KEY1 = 0x11; // 49
const int KEY2 = 0x12; // 50
const int KEY3 = 0x13; // 51
const int KEY4 = 0x14; // 52
const int KEY5 = 0x15; // 53
const int KEY6 = 0x16; // 54
const int KEY7 = 0x17; // 55
const int KEY8 = 0x18; // 56
const int KEY9 = 0x19; // 57
const int KEYWIDE = 0x77;
const int KEYDASH = 0xFF; // treat like a period 

// keys on the back of dual side alphanumeric remote control
const int KEYBACKUP = 0xE2;
const int KEYBACKDOWN = 0xE3;
const int KEYBACKLEFT = 0xE4;
const int KEYBACKRIGHT = 0xE5;

const int KEYBLUEX = 0xE7;
const int KEYYELLOWY = 0xE6;
const int KEYREDB = 0xE8;
const int KEYGREENA = 0xE9;

const int KEYENTER = 0xFE;
const int KEYBACKSPACE = 0xCA;
const int KEYSHIFTBACKSPACE = 0xCA;
const int KEYTAB = 0xCB;
const int KEYFORWARDSLASH = 0xDE; // 47
const int KEYAT = 0xA9; // 64
const int KEYCOLON = 0xD9; // 58
const int KEYSPACE = 0xCC; // 32
const int KEYCOMMA = 0xA7; // 44
const int KEYPERIOD = 0xA8; // 46
const int KEYDOTCOM = 0xDA;

const int KEYALTPERCENT = 0xAD; // 37
const int KEYALTPOUND = 0xAB; // 35
const int KEYALTDOLLAR = 0xAC; // 36
const int KEYALTEQUAL = 0xA5; // 61
const int KEYALTPLUS = 0xC4; // 43
const int KEYALTMINUS = 0xD4; // 45
const int KEYALTQUESTION = 0xD3; // 63
const int KEYALTLEFTBRACKET = 0xD8; // 91
const int KEYALTRIGHTBRACKET = 0xC0; // 93
const int KEYALTBACKSLASH = 0xC3; // 92
const int KEYALTASTERISK = 0xD1; // 42
const int KEYALTAMPERSAND = 0xAF; // 38
const int KEYALTEXCLAMATION = 0xAA; // 33
const int KEYALTBACKSPACE = 0xCD; // 127
const int KEYALTLESSTHAN = 0xD5; // 60
const int KEYALTGREATERTHAN = 0xD7; // 62
const int KEYALTUNDERSCORE = 0xC9; // 95
const int KEYALTLEFTPAREN = 0xC1; // 40
const int KEYALTRIGHTPAREN = 0xC2; // 41
const int KEYALTTILDE = 0xD2; // 126
const int KEYALTSEMICOLON = 0xA6; // 59
const int KEYALTSINGLEQUOTE = 0xC7; // 39
const int KEYALTDOUBLEQUOTE = 0xC8; // 34

const int KEYALT0 = 0xF1; // 48
const int KEYALT1 = 0xF2; // 49
const int KEYALT2 = 0xF3; // 50
const int KEYALT3 = 0xF4; // 51
const int KEYALT4 = 0xF5; // 52
const int KEYALT5 = 0xF6; // 53
const int KEYALT6 = 0xF7; // 54
const int KEYALT7 = 0xF8; // 55
const int KEYALT8 = 0xF9; // 56
const int KEYALT9 = 0xFA; // 57

const int KEYa = 0x5B; // 97
const int KEYb = 0x75; // 98
const int KEYc = 0x6E; // 99
const int KEYd = 0x5D; // 100
const int KEYe = 0x06; // 101
const int KEYf = 0x5E; // 102
const int KEYg = 0x5F; // 103
const int KEYh = 0x68; // 104
const int KEYi = 0x57; // 105
const int KEYj = 0x69; // 106
const int KEYk = 0x6A; // 107
const int KEYl = 0x6B; // 108
const int KEYm = 0x78; // 109
const int KEYn = 0x76; // 110
const int KEYo = 0x58; // 111
const int KEYp = 0x59; // 112
const int KEYq = 0x04; // 113
const int KEYr = 0x0C; // 114
const int KEYs = 0x5C; // 115
const int KEYt = 0x0D; // 116
const int KEYu = 0x29; // 117
const int KEYv = 0x70; // 118
const int KEYw = 0x05; // 119
const int KEYx = 0x6D; // 120
const int KEYy = 0x0F; // 121
const int KEYz = 0x6C; // 122

const int KEYA = 0x8C; // 65
const int KEYB = 0x9D; // 66
const int KEYC = 0x9B; // 67
const int KEYD = 0x8E; // 68
const int KEYE = 0x7B; // 69
const int KEYF = 0x8F; // 70
const int KEYG = 0x93; // 71
const int KEYH = 0x94; // 72
const int KEYI = 0x89; // 73
const int KEYJ = 0x95; // 74
const int KEYK = 0x96; // 75
const int KEYL = 0x97; // 76
const int KEYM = 0x9F; // 77
const int KEYN = 0x9E; // 78
const int KEYO = 0x8A; // 79
const int KEYP = 0x8B; // 80
const int KEYQ = 0x79; // 81
const int KEYR = 0x7C; // 82
const int KEYS = 0x8D; // 83
const int KEYT = 0x7D; // 84
const int KEYU = 0x7F; // 85
const int KEYV = 0x9C; // 86
const int KEYW = 0x7A; // 87
const int KEYX = 0x9A; // 88
const int KEYY = 0x7E; // 89
const int KEYZ = 0x99; // 90

// qrCode is 64x48px and contains http://mywebserver.local
const unsigned char qrCode [] PROGMEM = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xf0, 0x01, 0xe0, 0x1f, 0xc0, 0x07, 0xff, 0xff, 0xf0, 0x01, 0xe0, 0x1f, 0xc0, 0x07, 0xff, 
	0xff, 0xf3, 0xfd, 0x9f, 0x1c, 0xdf, 0xe7, 0xff, 0xff, 0xf2, 0x0d, 0x93, 0x78, 0xd8, 0x27, 0xff, 
	0xff, 0xf2, 0x0d, 0x93, 0x78, 0xd8, 0x27, 0xff, 0xff, 0xf2, 0x0d, 0x90, 0x04, 0xd8, 0x27, 0xff, 
	0xff, 0xf2, 0x0d, 0xfc, 0x9c, 0xd8, 0x27, 0xff, 0xff, 0xf2, 0x0d, 0xfc, 0x9c, 0xd8, 0x27, 0xff, 
	0xff, 0xf3, 0xfd, 0xfc, 0x87, 0xdf, 0xe7, 0xff, 0xff, 0xf3, 0xfd, 0xfc, 0x87, 0xdf, 0xe7, 0xff, 
	0xff, 0xf0, 0x01, 0x93, 0x64, 0xc0, 0x07, 0xff, 0xff, 0xff, 0xff, 0x80, 0xf8, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0x80, 0xf8, 0xff, 0xff, 0xff, 0xff, 0xf3, 0xfd, 0x9c, 0x00, 0x1e, 0x0f, 0xff, 
	0xff, 0xf3, 0xfd, 0x9c, 0x00, 0x1e, 0x0f, 0xff, 0xff, 0xf1, 0x83, 0x80, 0x03, 0x20, 0x0f, 0xff, 
	0xff, 0xff, 0xb0, 0x6c, 0x63, 0x06, 0xc7, 0xff, 0xff, 0xff, 0xb0, 0x6c, 0x63, 0x06, 0xc7, 0xff, 
	0xff, 0xf1, 0xf2, 0x13, 0xfb, 0x38, 0xf7, 0xff, 0xff, 0xf1, 0xf2, 0x13, 0xfb, 0x38, 0xf7, 0xff, 
	0xff, 0xfc, 0x0d, 0x90, 0xfb, 0xdf, 0xf7, 0xff, 0xff, 0xf2, 0x3e, 0x0f, 0x18, 0xe7, 0xcf, 0xff, 
	0xff, 0xf2, 0x3e, 0x0f, 0x18, 0xe7, 0xcf, 0xff, 0xff, 0xf2, 0x30, 0x7f, 0x80, 0x06, 0xc7, 0xff, 
	0xff, 0xf2, 0x30, 0x7f, 0x80, 0x06, 0xc7, 0xff, 0xff, 0xf2, 0x02, 0x63, 0x7f, 0x07, 0x37, 0xff, 
	0xff, 0xf3, 0xc0, 0x6f, 0x00, 0x01, 0x3f, 0xff, 0xff, 0xf3, 0xc0, 0x6f, 0x00, 0x01, 0x3f, 0xff, 
	0xff, 0xff, 0xff, 0x9f, 0xfc, 0xf9, 0x3f, 0xff, 0xff, 0xff, 0xff, 0x9f, 0xfc, 0xf9, 0x3f, 0xff, 
	0xff, 0xf0, 0x01, 0xf3, 0x64, 0xd8, 0xf7, 0xff, 0xff, 0xf3, 0xfd, 0xf0, 0xe0, 0xf9, 0xf7, 0xff, 
	0xff, 0xf3, 0xfd, 0xf0, 0xe0, 0xf9, 0xf7, 0xff, 0xff, 0xf2, 0x0d, 0xfc, 0x00, 0x00, 0x3f, 0xff, 
	0xff, 0xf2, 0x0d, 0xfc, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xf2, 0x0d, 0xef, 0x63, 0x1e, 0xc7, 0xff, 
	0xff, 0xf2, 0x0d, 0xf3, 0x7b, 0xe7, 0x37, 0xff, 0xff, 0xf2, 0x0d, 0xf3, 0x7b, 0xe7, 0x37, 0xff, 
	0xff, 0xf3, 0xfd, 0xe3, 0xe0, 0xc1, 0xf7, 0xff, 0xff, 0xf0, 0x01, 0xe3, 0xe0, 0xc1, 0xf7, 0xff, 
	0xff, 0xf0, 0x01, 0x8f, 0x7c, 0x3e, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

// fan1 is 64x48px
const unsigned char fan1 [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x80, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xf0, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x01, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xfc, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x01, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xfc, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x03, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xf0, 0x00, 0x00, 
  0x00, 0x01, 0xc0, 0x00, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x03, 0xe0, 0x00, 0x0e, 0x00, 0x00, 0x00, 
  0x00, 0x07, 0xf0, 0x0f, 0xe4, 0x00, 0x00, 0x00, 0x00, 0x07, 0xfc, 0x1f, 0xf0, 0x00, 0x00, 0x00, 
  0x00, 0x07, 0xff, 0x3f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0x3f, 0xf8, 0x00, 0x00, 0x00, 
  0x00, 0x07, 0xff, 0x3f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0x7f, 0xf8, 0x00, 0x00, 0x00, 
  0x00, 0x07, 0xff, 0x7f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0x3f, 0xf8, 0x00, 0x00, 0x00, 
  0x00, 0x07, 0xff, 0x3f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0x9f, 0xf3, 0x00, 0x00, 0x00, 
  0x00, 0x07, 0xff, 0x8f, 0xe3, 0xc0, 0x00, 0x00, 0x00, 0x03, 0xfe, 0x01, 0x0f, 0xe0, 0x00, 0x00, 
  0x00, 0x00, 0xf0, 0x00, 0x1f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xf0, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x7f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xf8, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x7f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xf8, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xf0, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x80, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// fan2 is 64x48px
const unsigned char fan2 [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x07, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x0f, 0xf0, 0x00, 0x00, 
  0x00, 0x00, 0x3c, 0x00, 0x1f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x1f, 0xf8, 0x00, 0x00, 
  0x00, 0x00, 0x7e, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x3f, 0xfe, 0x00, 0x00, 
  0x00, 0x00, 0xff, 0x80, 0x3f, 0xfe, 0x00, 0x00, 0x00, 0x01, 0xff, 0xc0, 0x0f, 0xfe, 0x00, 0x00, 
  0x00, 0x01, 0xff, 0xc7, 0xc7, 0xfe, 0x00, 0x00, 0x00, 0x01, 0xff, 0x9f, 0xe3, 0xfe, 0x00, 0x00, 
  0x00, 0x01, 0xff, 0x9f, 0xf3, 0xfc, 0x00, 0x00, 0x00, 0x01, 0xff, 0x3f, 0xf8, 0x00, 0x00, 0x00, 
  0x00, 0x01, 0xff, 0x3f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x3f, 0xf8, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x7f, 0x3f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xf0, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x1f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xe0, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x03, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x03, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xfc, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x07, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xfe, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x1f, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xfe, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x3f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xf8, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// fan3 is 64x48px
const unsigned char fan3 [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x80, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x1f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xc0, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x7f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x01, 0xff, 0xc0, 0x00, 0x30, 0x00, 0x00, 0x00, 0x01, 0xff, 0xe0, 0x01, 0xfc, 0x00, 0x00, 
  0x00, 0x03, 0xff, 0xe0, 0x03, 0xfe, 0x00, 0x00, 0x00, 0x03, 0xff, 0xe0, 0x07, 0xfe, 0x00, 0x00, 
  0x00, 0x01, 0xff, 0xc0, 0x1f, 0xfe, 0x00, 0x00, 0x00, 0x01, 0xff, 0x8f, 0x0f, 0xff, 0x00, 0x00, 
  0x00, 0x00, 0xff, 0x1f, 0xc7, 0xff, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x3f, 0xe7, 0xff, 0x00, 0x00, 
  0x00, 0x00, 0x1e, 0x7f, 0xf3, 0xff, 0x00, 0x00, 0x00, 0x00, 0x06, 0x7f, 0xf3, 0xff, 0x80, 0x00, 
  0x00, 0x00, 0x00, 0x7f, 0xf3, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xf3, 0xff, 0x80, 0x00, 
  0x00, 0x00, 0x00, 0x7f, 0xf0, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xf0, 0x1f, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x3f, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xc0, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x70, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xc0, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x03, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xc0, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x0f, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xc0, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x1f, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xc0, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x07, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0x80, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// fan4 is 64x48px
const unsigned char fan4 [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x01, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x1f, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x7f, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xfe, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x7f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xfe, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x7f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xfe, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x3f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xf0, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x0f, 0xc3, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x07, 0x9f, 0xc0, 0xff, 0x80, 0x00, 
  0x00, 0x00, 0x03, 0x3f, 0xf3, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x01, 0x7f, 0xf3, 0xff, 0xe0, 0x00, 
  0x00, 0x00, 0x00, 0x7f, 0xf9, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0xff, 0xf9, 0xff, 0xe0, 0x00, 
  0x00, 0x00, 0x00, 0xff, 0xfd, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0xff, 0xfd, 0xff, 0xe0, 0x00, 
  0x00, 0x00, 0x00, 0xff, 0xf9, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xf9, 0xff, 0xe0, 0x00, 
  0x00, 0x00, 0x00, 0x7f, 0xfb, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x01, 0x3f, 0xf3, 0xff, 0xe0, 0x00, 
  0x00, 0x00, 0x03, 0x9f, 0xe0, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x1f, 0xc7, 0x80, 0x3f, 0xc0, 0x00, 
  0x00, 0x00, 0xff, 0xf0, 0x00, 0x1f, 0xc0, 0x00, 0x00, 0x03, 0xff, 0xfe, 0x00, 0x0f, 0x80, 0x00, 
  0x00, 0x03, 0xff, 0xfe, 0x00, 0x07, 0x00, 0x00, 0x00, 0x03, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x03, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xfc, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xf8, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x07, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xe0, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// fan5 is 64x48px
const unsigned char fan5 [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x7f, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xf8, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x01, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xf8, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x01, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xf0, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x01, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xe0, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x80, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x70, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xc0, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x7f, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xe0, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xf3, 0xe0, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0xff, 0xf3, 0xf8, 0x00, 0x00, 0x00, 0x07, 0xfc, 0xff, 0xf3, 0xfc, 0x00, 0x00, 
  0x00, 0x0f, 0xfe, 0x7f, 0xe7, 0xfe, 0x00, 0x00, 0x00, 0x0f, 0xfe, 0x7f, 0xe7, 0xff, 0x00, 0x00, 
  0x00, 0x0f, 0xff, 0x3f, 0xcf, 0xff, 0x00, 0x00, 0x00, 0x07, 0xff, 0x8f, 0x1f, 0xfe, 0x00, 0x00, 
  0x00, 0x07, 0xff, 0xc0, 0x1f, 0xfe, 0x00, 0x00, 0x00, 0x07, 0xff, 0xe0, 0x0f, 0xfe, 0x00, 0x00, 
  0x00, 0x03, 0xff, 0xc0, 0x0f, 0xfc, 0x00, 0x00, 0x00, 0x03, 0xff, 0x80, 0x07, 0xfc, 0x00, 0x00, 
  0x00, 0x01, 0xff, 0x80, 0x07, 0xf8, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x03, 0xf0, 0x00, 0x00, 
  0x00, 0x00, 0x7c, 0x00, 0x03, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x01, 0xc0, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

  const char webpageTop[] = R"==(
  <!DOCTYPE html>
  <html lang="en">  
  <head> 
  <title>Temperature Report</title> 
  <style type="text/css">
  h1 {font-size:40pt;}
  h2 {font-size:32pt;}
  table, th, td {border-width:1px;border-style:solid;border-color:black;}
  table {font-size:32pt;width:80%;background-color:white;color:black;border-collapse:separate;}
  thead th {font-size:32pt;position:sticky;top:0;background-color:black;color:white;height:120pt;}
  button{font-size:32pt;text-align:center;background-color:black;color:white;width:80%;height:120pt;}   
  td {font-size:32pt;text-align:center;} 
  .relayMode{font-size:32pt;text-align:center;background-color:black;color:white;width:80%;height:120pt;} 
  .reportTableContainer{height:40vh;overflow-y:auto;}
  .reportFilenames {font-size:32pt;text-align:center;background-color:black;color:white;width:80%;height:120pt;} 
  .setting {font-size:24pt;text-align:center;height:120pt;width:60%;}
  .hidden {display:none;}  
  .change {font-size:24pt;text-align:center;background-color:black;color:white;height:120pt;width:40%;}
  .message {font-size:32pt;text-align:center;background-color:lightgreen;color:black;border:2px solid black;width:80%;}
  </style>
  </head>
  <body style=background-color:lightgray; onload="refreshReport()">
  <center>
  )==";

  const char globalVariables[] = R"==(
  let currentTemperature = "";  
  let currentHumidity = "";
  let displayTemperature = true;
  let scrollTimePosition = "top";         
  let scrollTemperaturePosition = "high";  
  let temperatureLowRow = 0;
  let temperatureHighRow = 0;
  let scrollHumidityPosition = "high";     
  let humidityLowRow = 0;
  let humidityHighRow = 0;
  )==";          

  const char unsolicitedEventsHandler[] = R"==(
  if (!!window.EventSource)
    {
    var source = new EventSource('/events');
    // execute when mobile device's web browser navigates to our web server url  
    source.addEventListener('open', function(e)
      {
      xmlRequest("/refreshReport"); 
      }, false);
      
    // execute when our web server sends an unsolicited message to our browser's webpage  
    source.addEventListener('message', function(e)
      {
      // in our implementation, the web server sends some text to be displayed
      // This would be a good way to automatically send diagnostic messages  
      // when resources are becoming low or other software or hardware
      // failures needs to be reported.
      document.getElementById("messageId").innerHTML = e.data;
      }, false);
    source.addEventListener('updateTemperature', function(e)
      {        
      currentTemperature = e.data;
      }, false);
    source.addEventListener('updateHumidity', function(e)
      {        
      currentHumidity = e.data;
      }, false);
    source.addEventListener('refreshReport', function(e)
      {        
      xmlRequest("/refreshReport");      
      }, false);               
    }
  )==";

  const char displayTemperatureAndHumidityFunction[] = R"==(
  function displayTemperatureAndHumidity()
    {
    if (displayTemperature)
      {
      document.getElementById("messageId").innerHTML = "Current Temperature " + currentTemperature;  
      displayTemperature = false;
      } 
    else
      {
      document.getElementById("messageId").innerHTML = "Current Humidity " + currentHumidity;  
      displayTemperature = true;
      }
    }
  const intervalId = setInterval(displayTemperatureAndHumidity, 5000);
  )==";        

  const char xmlRequestFunction[] = R"==(
  function xmlRequest(url)
    {  
    var xhttp = new XMLHttpRequest();     
    xhttp.open("GET", url, true);
    xhttp.onreadystatechange=xmlResponse;       
    xhttp.send();
    }
  )==";   

  const char xmlResponseFunction[] = R"==(
  function xmlResponse()
    {
    if (this.readyState == 4 && this.status == 200)
      {
      let xmlData = this.responseXML;

      xmlElement = xmlData.getElementsByTagName("messageId");
      xmlValue = xmlElement[0].firstChild.nodeValue;
      document.getElementById("messageId").textContent=xmlValue;

      xmlElement = xmlData.getElementsByTagName("temperatureId");
      xmlValue = xmlElement[0].firstChild.nodeValue;
      currentTemperature=xmlValue;

      xmlElement = xmlData.getElementsByTagName("humidityId");
      xmlValue = xmlElement[0].firstChild.nodeValue;
      currentHumidity=xmlValue;                   
 
      // uses innerHTML because reportDataId contains nested xml data
      xmlElement = xmlData.getElementsByTagName("reportDataId");
      if (xmlElement.length != 0)
        {
        xmlValue = xmlElement[0].firstChild.nodeValue;
        document.getElementById("reportDataId").innerHTML=xmlValue;
        }

      // uses innerHTML because reportFilenamesId contains nested xml data
      xmlElement = xmlData.getElementsByTagName("reportFilenamesId");
      if (xmlElement.length != 0)
        {
        xmlValue = xmlElement[0].firstChild.nodeValue;
        document.getElementById("reportFilenamesId").innerHTML=xmlValue;
        }

      xmlElement = xmlData.getElementsByTagName("relayModeId");
      xmlValue = xmlElement[0].firstChild.nodeValue;
      listbox = document.getElementById("relayModeId");
      for (let i = 0; i < listbox.options.length; i++)
        {
        if (listbox.options[i].value == xmlValue)
          {
          listbox.selectedIndex = i;  
          listbox.options[i].selected = true;
          }
        else
          {
          listbox.options[i].selected = false;
          }
        }

      xmlElement = xmlData.getElementsByTagName("relayStateId");
      xmlValue = xmlElement[0].firstChild.nodeValue;
      if (xmlValue == "Relay is Off")
        document.getElementById("relayModeId").style.color = "salmon";   
      else
        document.getElementById("relayModeId").style.color = "lightgreen";       

      xmlElement = xmlData.getElementsByTagName("requestId");
      xmlValue = xmlElement[0].firstChild.nodeValue;
      if (xmlValue == "refreshReport")
        {
        updateReportTable();        
        }
      window.scrollTo({top:0,behavior:'smooth'});                 
      }      
    }
  )==";

  const char updateReportTableFunction[] = R"==(
  // when web page finishes loading, highlight the lowest and highest temperatures and humidities
  function updateReportTable()
    {        
    let LOW_THRESHOLD = 999;
    let HIGH_THRESHOLD = -999
    let temperatureLow = LOW_THRESHOLD;
    let temperatureHigh = HIGH_THRESHOLD;
    let humidityLow = LOW_THRESHOLD;
    let humidityHigh = HIGH_THRESHOLD;

    let temperatureLowRowSet = false;
    let temperatureHighRowSet = false;
    let humidityLowRowSet = false;
    let humidityHighRowSet = false;

    // find the day's lowest and highest temperature and humidity
    let table = document.getElementById("reportTableId");    
    for (let rowNumber = 1; rowNumber < table.rows.length; rowNumber++)
      {       
      let row = table.rows[rowNumber];
      let temperatureColumn = row.cells[1];
      let temperatureContent = temperatureColumn.textContent.trim();
      if (temperatureContent < temperatureLow)
        temperatureLow = temperatureContent;       
      if (temperatureContent > temperatureHigh)
        temperatureHigh = temperatureContent;
      let humidityColumn = row.cells[2];
      let humidityContent = humidityColumn.textContent.trim();    
      if (humidityContent < humidityLow)
        humidityLow = humidityContent;    
      if (humidityContent > humidityHigh)
        humidityHigh = humidityContent;   
      }

    // highlight all the lowest and highest temperature and humidity table cells
    for (let rowNumber = 1; rowNumber < table.rows.length; rowNumber++)
      { 
      // cells[0] is time column, cells[1] is temperature column, cells[2] is humidity column   
      let row = table.rows[rowNumber];
      let temperatureColumn = row.cells[1];
      let temperatureContent = temperatureColumn.textContent.trim();
      if (temperatureContent == temperatureLow)
        {  
        if (!temperatureLowRowSet)
          { 
          temperatureLowRow = rowNumber;    
          temperatureLowRowSet = true;
          }
        row.cells[1].style.backgroundColor = "lightcyan";
        }      
      if (temperatureContent == temperatureHigh)
        { 
        if (!temperatureHighRowSet)
          {
          temperatureHighRow = rowNumber;
          temperatureHighRowSet = true;
          }
              
        row.cells[1].style.backgroundColor = "salmon";  
        }      
      let humidityColumn = row.cells[2];
      let humidityContent = humidityColumn.textContent.trim();
      if (humidityContent == humidityLow)
        {  
        if (!humidityLowRowSet)
          {
          humidityLowRow = rowNumber;
          humidityLowRowSet = true;
          }        
        row.cells[2].style.backgroundColor = "lightcyan";
        }      
      if (humidityContent == humidityHigh)
        { 
        if (!humidityHighRowSet)
          {
          humidityHighRow = rowNumber;
          humidityHighRowSet = true;
          }                     
        row.cells[2].style.backgroundColor = "salmon";
        }      
      }
    let lastRow = table.rows[table.rows.length-1];
    if (lastRow)
      lastRow.scrollIntoView({ behavior: 'smooth', block: 'end' });
    }
    )==";   

  const char scrollTimeFunction[] = R"==(
  function scrollTime()
    {
    if (scrollTimePosition == "top")
      {
      scrollTimePosition = "bottom"; 
      let reportTable = document.getElementById("reportTableId");
      let rowCount = reportTable.rows.length; 
      let lastRow = reportTable.rows[rowCount-1];
      if (lastRow)
        lastRow.scrollIntoView({ behavior: 'smooth', block: 'end' });        
      }    
    else if (scrollTimePosition == "bottom")
      {
      scrollTimePosition = "top";  
      let reportTable = document.getElementById("reportTableId");
      let firstRow = reportTable.rows[0];
      if (firstRow)
        firstRow.scrollIntoView({ behavior: 'smooth', block: 'end' });       
      }  
    }
  )==";

  const char scrollTemperatureFunction[] = R"==(
  function scrollTemperature()
    {
    if (scrollTemperaturePosition == "low")
      {
      scrollTemperaturePosition = "high"; 
      let reportTable = document.getElementById("reportTableId");
      let highRow = reportTable.rows[temperatureHighRow];      
      if (highRow)
        highRow.scrollIntoView({ behavior: 'smooth', block: 'end' });        
      }    
    else if (scrollTemperaturePosition == "high")
      {
      scrollTemperaturePosition = "low";  
      let reportTable = document.getElementById("reportTableId");
      let lowRow = reportTable.rows[temperatureLowRow];
      if (lowRow)
        lowRow.scrollIntoView({ behavior: 'smooth', block: 'end' });       
      }  
    }
  )==";

  const char scrollHumidityFunction[] = R"==(
  function scrollHumidity()
    {
    if (scrollHumidityPosition == "low")
      {
      scrollHumidityPosition = "high"; 
      let reportTable = document.getElementById("reportTableId");
      let highRow = reportTable.rows[humidityHighRow];
      if (highRow)
        highRow.scrollIntoView({ behavior: 'smooth', block: 'end' });        
      }    
    else if (scrollHumidityPosition == "high")
      {
      scrollHumidityPosition = "low";  
      let reportTable = document.getElementById("reportTableId");
      let lowRow = reportTable.rows[humidityLowRow];
      if (lowRow)
        lowRow.scrollIntoView({ behavior: 'smooth', block: 'end' });       
      }  
    }
  )==";

  const char getReportFilenameFunction[] = R"==(
  function getReportFilename()
    {
    let element = document.getElementById("reportFilenamesId");
    return element.options[element.selectedIndex].text; 
    }
  )==";

  const char selectReportFunction[] = R"==(
  function selectReport(selectElement)  
    { 
    let reportFilename = getReportFilename();
    xmlRequest("/selectReport?reportFilename="+reportFilename);
    }
  )==";

  const char relayModeChangeFunction[] = R"==(
  function relayModeChange(relayMode)
    {
    xmlRequest("/relayModeChange?relayMode="+relayMode);
    }
  )==";  

  const char refreshReportFunction[] = R"==(
  function refreshReport()
    {
    xmlRequest("/refreshReport"); 
    }
  )==";

  const char deleteReportFunction[] = R"==(
  function deleteReport()
    {
    let reportFilename = getReportFilename();
    let confirmed = confirm("Are you sure you want to delete report\n"+reportFilename);
    if (confirmed)
      xmlRequest("/deleteReport?reportFilename="+reportFilename);
    }
  )==";

  const char downloadReportFunction[] = R"==(
  function downloadReport()
    {
    window.location.href = '/download';    
    } 
  )==";  

  const char calibrateTemperatureFunction[] = R"==(
  function calibrateTemperature()
    {
    while (true)
      {
      let temperature = prompt("Calibrate Temperature\nEnter current accurate temperature","");
      if (temperature === null)
        break;  
      if (temperature > 0 && temperature <= 130)
        {
        xmlRequest("/calibrateTemperature?temperature="+temperature);  
        break;    
        }
      }
    }
  )==";

  const char calibrateHumidityFunction[] = R"==(
  function calibrateHumidity()
    {      
    while (true)
      {
      let humidity = prompt("Calibrate Humidity\nEnter current accurate humidity","");
      if (humidity === null)
        break;  
      else if (humidity > 0 && humidity <= 100)
        {
        xmlRequest("/calibrateHumidity?humidity="+humidity);  
        break;    
        }
      }
    } 
  )==";

  const char webpageBottom[] = R"==(     
  </center>
  </body>
  </html>
  )==";  

void setup()
  {
  Serial.begin(9600);
  delay(2000);

  // initialize the Espressif Systems ESP32-C6-DEVKITC-1-N8 Microcontroller RGB led  
  ledBegin();

  // initialize I2C communication
  Wire.begin(SDA_PIN,SCL_PIN);

  // initialize oled ssd1306 display
  oledBegin();

  // intentionally delay so ESP32 WiFi has time to reset itself after loading a new sketch
  oledMessages("POWER ON", "IN", "PROGRESS", "", "", "", "", 10000);  

  // cycle thru several microcontroller onboard led colors
  ledColor(RGB_PURPLE);
  delay(1500);
  ledColor(RGB_BLUE);
  delay(1500);
  ledColor(RGB_CYAN);
  delay(1500);
  ledColor(RGB_GREEN);
  delay(1500);
  ledColor(RGB_YELLOW);
  delay(1500);
  ledColor(RGB_MAGENTA);
  delay(1500);
  ledColor(RGB_RED);
  delay(1500);
  ledColor(RGB_WHITE);
  delay(1500);

  // display the blue led while rest of setup function is running
  ledColor(RGB_BLUE);

  // display sample animation of spinning fan
  for (int i=1; i <= 20; i++)
    {
    oledDisplayBitmap("DEMO10"," ","       Temperature","       Monitor","       System"," ", 12, 16, fan1, 64, 48, 50);
    oledDisplayBitmap("DEMO10"," ","       Temperature","       Monitor","       System"," ", 12, 16, fan2, 64, 48, 50);
    oledDisplayBitmap("DEMO10"," ","       Temperature","       Monitor","       System"," ", 12, 16, fan3, 64, 48, 50);
    oledDisplayBitmap("DEMO10"," ","       Temperature","       Monitor","       System"," ", 12, 16, fan4, 64, 48, 50);
    oledDisplayBitmap("DEMO10"," ","       Temperature","       Monitor","       System"," ", 12, 16, fan5, 64, 48, 50);
        }    

  // display sample copyright on oled screen for 3 seconds 
  oledMessages("DEMO10", "Version 2026.10.18", "Copyright (c) 2026", "Larry Anderson", "", "", "", 3000); 

  // initialize relay to off
  pinMode(RELAY_PIN,OUTPUT);
  relayOff();   

  // initialize infrared receiver
  IrReceiver.begin(IR_RECEIVE_PIN, 0);

  // load an array with infrared remote control key codes
  irLoadKeyCodes();  

  // initialize the serial peripheral interface protocol used to communicate with the microsd adapter 
  SPI.begin(SCK, MISO, MOSI, CS);
  if (!SD.begin(CS))
    {
    oledMessages("ATTENTION", "MicroSD", "Error", "Check memory card", "", "", "", 0);
    ledColor(RGB_RED);
    while(true);
    }    

  // retrieve plain text configuration settings stored on the microsd memory card.
  // if they don't exist, write the defaults defined in this sketch to the memory card.  
  settingsLoad();

  // log on to an existing wifi network.
  // if you want your ESP32 to be able to connect to your smtp server account
  // to send emails containing the ccyy-mm-dd.csv file as an attachment,
  // use a wifi network that supports access to the internet.
  // Do a google search on how to setup a Simple Mail Transfer Protocol (smtp) server account.
  // I set mine up using my google gmail account
  bool wifiLogonInProgress = true;
  while (wifiLogonInProgress)
    {    
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSSID, wifiPassword);
    oledMessages("WIFI", "Logging on", wifiSSID, " ", "", "", "", 10000);
    if (WiFi.status() == WL_CONNECTED)
      wifiLogonInProgress = false;
    else
      {
      oledMessages("ATTENTION", "Logging on", "failed.", "Use remote control", "and menu", "to change the", "wifi SSID & Password", 10000); 
      // use the remote control to change the wifi ssid and wifi password and then the wifiLogonInProgress loop retries the wifi logon again
      mainMenu();  
      }  
    }

  serverIp = WiFi.localIP();  
  sprintf(serverIpDisplay,"%d.%d.%d.%d",serverIp[0],serverIp[1],serverIp[2],serverIp[3]); 
  oledMessages("WIFI", "Logon", "Successful", " ", serverIpDisplay, "", "", 5000);

  sprintf(serverIpDisplay,"%d.%d.%d.%d",serverIp[0],serverIp[1],serverIp[2],serverIp[3]); 
  oledMessages("INFO", "View temperature", "reports, using cell", "phone's web browser", "and navigate to the", "web page url", "mywebserver.local", 10000);

  // if ds3231 real time clock initialization fails
  if (!rtc.begin())
    {
    oledMessages("ATTENTION", "Clock failed.", "Check clock battery.", "To set the clock,", "press any key", "on the", "remote control.", 0);
    waitForUser();  
    clockUpdate();
    ESP.restart();  
    }

  // if ds3231 real time clock lost power
  if (rtc.lostPower())
    {
    oledMessages("ATTENTION", "Clock lost power", "Check clock battery.", "To set the clock,", "press any key", "on the", "remote control.", 0);
    waitForUser();  
    clockUpdate();
    ESP.restart();        
    }

  // get current date and time for todays ccyy-mm-dd.csv filepath and filename
  getDateTimeFilename();

  // if todays csv file does not exist yet, create it now
  if (!SD.exists(csvFilepath))
    {
    file = SD.open(csvFilepath, FILE_WRITE);
    file.println("Time,Temperature,Humidity"); 
    file.close();
    temperatureLow = LOW_THRESHOLD;
    temperatureHigh = HIGH_THRESHOLD;
    humidityLow = LOW_THRESHOLD;
    humidityHigh = HIGH_THRESHOLD;
    csvCount = 1;     
    }
  // else count existing records in the file so we resume displaying an accurate daily record count
  else if (SD.exists(csvFilepath))
    {
    csvCount = 0;  
    file = SD.open(csvFilepath, FILE_READ);
    if (file)
      {       
      while (file.available())
        {
        String tempString = file.readStringUntil('\n');
        csvCount++; 
        }
      file.close();        
      }   
    }
  // don't include the csv heading record in the current record count   
  if (csvCount > 0)
    csvCount--;  

  // create webpage and its content needed by our web server to send to a web brower request  
  reportFilenameUpdate(csvFilename);
  reportDataUpdate();
  reportFilenamesUpdate();
  webpageCreate(); 

  // initialize bme280 temperature sensor
  bme.begin(0x76);
  
  // execute when user initially navigates their web browser to http://mywebserver.local
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    {           
    request->send_P(200, "text/html", webpage);   
    });

  // route for events
  events.onConnect([](AsyncEventSourceClient *client)
    {
    if(client->lastId())
      {
      //Serial.printf("Client reconnected! Last message ID: %u\n", client->lastId());
      }
    client->send("You are connected to ESP32!", NULL, millis(), 0);
    });
  server.addHandler(&events);

  // execute when user selects report filename from web page listbox
  server.on("/selectReport", HTTP_GET, [](AsyncWebServerRequest *request)
    {  
    string = request->getParam("reportFilename")->value();
    string.toCharArray(reportFilename,sizeof(reportFilename));
    reportFilenameUpdate(reportFilename);
    sendXml(request,"selectReport","Selected temperature report");    
    });

  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request)
    {
    File file = SD.open(reportFilepath, FILE_READ); // do not close the file!
    request->send(file, reportFilepath, "application/octet-stream", true);
    });    

  // execute when web page changes or deletes a report
  server.on("/refreshReport", HTTP_GET, [](AsyncWebServerRequest *request)
    {  
    sendXml(request,"refreshReport","Refreshed temperature report"); 
    });

  // execute when user selects web page delete report button
  server.on("/deleteReport", HTTP_GET, [](AsyncWebServerRequest *request)
    {         
    if (SD.exists(reportFilepath))
      {
      SD.remove(reportFilepath);  
      }     
    reportFilenameUpdate(csvFilename);   
    sendXml(request,"deleteReport","Deleted temperature report"); 
    });

  // execute when user selects web page calibrate temperature button
  server.on("/calibrateTemperature", HTTP_GET, [](AsyncWebServerRequest *request)
    {
    string = request->getParam("temperature")->value();
    int currentTemperature = string.toInt();
    temperatureCalibration = currentTemperature - rawTemperature;
    sprintf(calibrateTemperature,"%d",temperatureCalibration);
    settingWrite(calibrateTemperatureFilepath,calibrateTemperature,false);      
    sendXml(request,"calibrateTemperature","Calibrated temperature"); 
    });

  // execute when user selects web page calibrate humidity button
  server.on("/calibrateHumidity", HTTP_GET, [](AsyncWebServerRequest *request)
    {     
    string = request->getParam("humidity")->value();
    int currentHumidity = string.toInt();
    humidityCalibration = currentHumidity - rawHumidity;
    sprintf(calibrateHumidity,"%d",humidityCalibration);
    settingWrite(calibrateHumidityFilepath,calibrateHumidity,false);      
    sendXml(request,"calibrateHumidity","Calibrated humidity"); 
    });

  // execute when user selects a relay mode
  server.on("/relayModeChange", HTTP_GET, [](AsyncWebServerRequest *request)
    {
    string = request->getParam("relayMode")->value();
    string.toCharArray(relayMode,sizeof(relayMode));
    settingWrite(relayModeFilepath,relayMode,false);
    relayModeCheck();                 
    sendXml(request,"relayModeChange","Changed Relay Mode");                         
    });                                       

  // add over-the-air update capability to our ESP32 web server
  updateServer.setup(&server);
  updateServer.onUpdateBegin = [](const UpdateType type, int &result)
    {
    // abort the update like this if you need to:
    // result = UpdateResult::UPDATE_ABORT;        
    // Serial.println("Update started : " + String(type));
    };
  updateServer.onUpdateEnd = [](const UpdateType type, int &result)
    {
    // Serial.println("Update finished : " + String(type) + " result: " + String(result));
    };       

  // start the web server so it is ready to be accessed by a web browser on cell, tablet, laptop, or desktop
  server.begin();

  // use the multicast domain name system protocol so a web browser user can access the web server by its name opposed to its ip address 
  MDNS.begin("mywebserver");          // web browser user enters http://mywebserver.local opposed to the web server ip address
  MDNS.addService("http","tcp",80);

  // display black led (off) to signify setup is done and sketch is entering its normal repeating loop cycle
  ledColor(RGB_OFF);              
  }  

// the last statements in this function delay(1000) causes this loop function to repeat execution at precisely 1 second intervals 
void loop()
  {
  unsigned long loopStartMillis;
  unsigned long loopEndMillis;
  unsigned long loopTotalMillis;    
  
  //boolean rc = false;
  loopStartMillis = millis();

  // get current date and time and generate todays ccyy-mm-dd.csv filepath and filename  
  getDateTimeFilename();

  // if web browser requested web page to be refreshed
  if (refreshRequested)
    {  
    reportDataUpdate();
    reportFilenamesUpdate();  
    refreshRequested = false;
    events.send(csvFilename, "refreshReport", millis());  
    }  

  // if it's the end of today, automatically reboot the ESP32 to eliminate excessive fragmented memory
  if ((datetime.hour() == 23) && (datetime.minute() == 59) && (datetime.second() == 59))        
    { 
    ESP.restart();       
    }

  // look for temperature changes every 60 seconds
  if (datetime.second() == 0)
    {
    // if todays csv does not exist, force a new one to be created  
    if (!SD.exists(csvFilepath))        
      dataChanged = true;
    // else only write csv record to existing csv file when temperature or humidity changes
    else
      dataChanged = false;

    // get temperature and convert from centigrade to fahrenheit
    dataFloat = bme.readTemperature();
    dataFloat = ((dataFloat * 9.0f) / 5.0f) + 32.0f;
    rawTemperature = (int)floor(dataFloat+0.0001); // 78.0 thru 78.9 becomes 78
    temperature = rawTemperature + temperatureCalibration;
    sprintf(temperatureChar, "%d", temperature);
    if (temperature != temperaturePrevious)
      {
      dataChanged = true;
      sprintf(messageWorkarea, "<temperatureId>%s%s</temperatureId>", temperatureChar,"\xC2\xB0");
      events.send(messageWorkarea, "updateTemperature", millis());
      }
    temperaturePrevious = temperature;
    if (temperature < temperatureLow)
      temperatureLow = temperature;
    if (temperature > temperatureHigh)
      temperatureHigh = temperature;

    // display different microcontroller onboard LED color based on air temperature ranges  
    if (temperature < 70)
      ledColor(RGB_BLUE);
    else if (temperature < 80)
      ledColor(RGB_CYAN);
    else if (temperature < 90)
      ledColor(RGB_YELLOW);
    else if (temperature < 100)
      ledColor(RGB_MAGENTA);
    else
      ledColor(RGB_RED);

    // turn relay on or off 
    relayModeCheck();     
 
    // get humidity
    dataFloat = bme.readHumidity();
    rawHumidity = (int)floor(dataFloat+0.0001); // 78.0 thru 78.9 becomes 78
    humidity = rawHumidity + humidityCalibration;
    sprintf(humidityChar, "%d", humidity);
    if (humidity != humidityPrevious)
      {
      sprintf(messageWorkarea, "<humidityId>%s%s</humidityId>", humidityChar, "%");
      events.send(messageWorkarea, "updateHumidity", millis());
      }
    humidityPrevious = humidity;    
    if (humidity < humidityLow)
      humidityLow = humidity;
    if (humidity > humidityHigh)
      humidityHigh = humidity;             

    // get altitude feet, just to show how it is done
    dataFloat = bme.readAltitude(SEALEVELPRESSURE_HPA);
    altitude = (int)floor(dataFloat+0.0001);
    sprintf(altitudeChar, "%d", altitude);

    // if temperature changed, write a csv file record
    if (dataChanged == true)
      {
      // if todays csv file does not exist, start creating it   
      if (!SD.exists(csvFilepath))
        {
        file = SD.open(csvFilepath, FILE_WRITE);
        file.println("Time,Temperature,Humidity"); 
        temperatureLow = LOW_THRESHOLD;
        temperatureHigh = HIGH_THRESHOLD;
        humidityLow = LOW_THRESHOLD;
        humidityHigh = HIGH_THRESHOLD;
        csvCount = 0;
         }
      // else open todays existing csv file in append mode    
      else
        {
        file = SD.open(csvFilepath, FILE_APPEND);  
        }     
      // create csv record containing hh:mm,temperature,humidity
      // and append it to todays csv file  
      sprintf(csvRecord, "%02d:%02d", datetime.hour(), datetime.minute());  // hh:mm
      strcat(csvRecord,",");
      strcat(csvRecord,temperatureChar);
      strcat(csvRecord,",");
      strcat(csvRecord,humidityChar);   
      file.println(csvRecord);
      csvCount++;
      file.close();
      // if the temperature report for the current day is currently displayed, display the updated report 
      if (strcmp(reportFilename,csvFilename) == 0)
        {
        reportDataUpdate();
        reportFilenamesUpdate();
        events.send(csvFilename, "refreshReport", millis());    
        }               
      } 
    }

  // every 2 seconds display date, time, temperature, rh, min temp, max temp, min rh, max rh, csv record count
  if (datetime.second()%2 == 0)
    {
    yield();  
    displayCycle++;
    if (displayCycle > 10)
      displayCycle = 1;

    if (displayCycle == 1)
      {
      // display qr code for cell phone camera quick connect to web server capability
      oledDisplayBitmap("DEMO10","","","","","", 32, 16, qrCode, 64, 48, 0);         
      }
    else if (displayCycle == 2)
      {
      sprintf(displayArea, "%04d-%02d-%02d", datetime.year(), datetime.month(), datetime.day());      // ccyy-mm-dd
      oledMessages("DATE", displayArea, "", "", "", "", "", 0);    
      }
    else if (displayCycle == 3)
      {
      sprintf(displayArea, "%02d:%02d:%02d", datetime.hour(), datetime.minute(), datetime.second());  // hh:mm:ss
      oledMessages("TIME", displayArea, "", "", "", "", "", 0);
      }        
    else if ((displayCycle == 4) && (temperatureHigh != HIGH_THRESHOLD))
      {
      oledMessages("TEMP", temperatureChar, "DEGREES", "", "", "", "", 0);          
      }
    else if ((displayCycle == 5) && (humidityHigh != HIGH_THRESHOLD))
       {
       oledMessages("RH", humidityChar, "PERCENT", "", "", "", "", 0);         
      }
    else if ((displayCycle == 6) && (temperatureLow != LOW_THRESHOLD))
      {
      sprintf(displayArea, "%d", temperatureLow);  
      oledMessages("MIN TEMP", displayArea, "DEGREES", "", "", "", "", 0);        
      } 
    else if ((displayCycle == 7) && (temperatureHigh != HIGH_THRESHOLD))
      {
      sprintf(displayArea, "%d", temperatureHigh);  
      oledMessages("MAX TEMP", displayArea, "DEGREES", "", "", "", "", 0);            
      }
    else if ((displayCycle == 8) && (humidityLow != LOW_THRESHOLD))
      {
      sprintf(displayArea, "%d", humidityLow);
      oledMessages("MIN RH", displayArea, "PERCENT", "", "", "", "", 0);           
      }
    else if ((displayCycle == 9) && (humidityHigh != HIGH_THRESHOLD))
      {
      sprintf(displayArea, "%d", humidityHigh);        
      oledMessages("MAX RH", displayArea, "PERCENT", "", "", "", "", 0);           
      }
    else if (displayCycle == 10)
      {
      sprintf(displayArea, "%d", csvCount);  
      oledMessages("RECORDED", displayArea, "RECORDS", "", "", "", "", 0);           
      }                                 
    }
  // wait 1 second before repeating the loop function again
  loopEndMillis=millis();
  loopTotalMillis=loopEndMillis-loopStartMillis;
  if (loopTotalMillis < 1000) 
    irDelay((int)(int)1000-(int)loopTotalMillis);
  }

// datetime must contain ccyy-mm-dd hh:mm:ss
void clockSet(char * datetime)
  {
  char ccyyChars[4 + 1] = "";
  char monthChars[2 + 1] = "";
  char dayChars[2 + 1] = "";  
  char hourChars[2 + 1] = "";
  char minuteChars[2 + 1] = "";
  char secondChars[2 + 1] = "";
  bool validDateTime = true;
  if (strlen(datetime) >= strlen("ccyy-mm-dd hh:mm:ss"))
    {
    copyChars(datetime, 0, 4, ccyyChars);  
    copyChars(datetime, 5, 2, monthChars);
    copyChars(datetime, 8, 2, dayChars);    
    copyChars(datetime, 11, 2, hourChars);
    copyChars(datetime, 14, 2, minuteChars);
    copyChars(datetime, 17, 2, secondChars);
    // rough check for valid date and time 
    if (((atoi(monthChars) < 1) || ((atoi(monthChars) > 12) )))
      validDateTime = false;
    if (((atoi(dayChars) < 1) || ((atoi(dayChars) > 31))))
      validDateTime = false;
    if (((atoi(ccyyChars) < 1970) || ((atoi(ccyyChars) > 3000))))
      validDateTime = false;
    if (((atoi(hourChars) < 0) || ((atoi(hourChars) > 23))))
      validDateTime = false;
    if (((atoi(minuteChars) < 0) || ((atoi(minuteChars) > 59))))
      validDateTime = false;
    if (((atoi(secondChars) < 0) || ((atoi(hourChars) > 59))))
      validDateTime = false;
    }
  else
    {
    validDateTime = false;  
    }   
  if (validDateTime)
    {
    rtc.adjust(DateTime(atoi(ccyyChars), atoi(monthChars), atoi(dayChars), atoi(hourChars), atoi(minuteChars), atoi(secondChars)));
    oledMessages("CLOCK", "SET", "SUCCESSFUL", "", "", "", "", 5000);
    // force restart so a date change can create a new .csv file 
    ESP.restart();
    } 
  else
    {
    // display an error message for 5 seconds before user can try again
    oledMessages("ATTENTION", "invalid date & time", "must specify time in", "24 hour format", "ccyy-mm-dd hh:mm:ss","","", 5000);
    }    
  }

  // get current date and time and generate current ccyy-mm-dd.csv filepath and filename
  void getDateTimeFilename()
    {
    // get current date and time from ds3231 real time clock
    datetime = rtc.now();

    // todays csv filepath /ccyy-mm-dd.csv
    sprintf(csvFilepath,"%s%04d-%02d-%02d%s","/", datetime.year(), datetime.month(), datetime.day(),".csv");

    // todays csv filename ccyy-mm-dd.csv 
    sprintf(csvFilename,"%04d-%02d-%02d%s",datetime.year(), datetime.month(), datetime.day(),".csv");
    }  

// while delaying for time to pass, every 100 milliseconds, check to see if user pressed an infrared remote control key
boolean irDelay(int milliSeconds)
  {
  // during delay, process remote control keys  
  while (milliSeconds > 0) 
    {
    // if we did process one or more remote control keys, abort remaining delay  
    if (irGetSensor())
      return true;
    delay(100);
    milliSeconds -= 100;
    }
  return false;    
  }

// when user presses either infrared remote control MENU or PAUSE button
boolean irGetSensor()
  {
  boolean rc = false;
  rc = irGet();  
  if (rc)
    {        
    if (irKey == KEYMENU)
      mainMenu();     // display the oled menu
    else if (irKey == KEYPAUSE)
      {  
      waitForUser();  // pause processing til any remote key pressed
      }
    } 
  return rc;  
  }

// return true when user pressed an infrared remote control key, else return false 
boolean irGet()
  {
  boolean rc = (IrReceiver.decode());
  if (rc)
    {
    // Ignore repeat codes to avoid multiple actions for a single button press
    if ((IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT))
      {
      IrReceiver.resume(); // resume receiving the next signal
      return false;
      }    

    // extract the command (key code) from the decoded data
    unsigned long cmd = IrReceiver.decodedIRData.command;
    irKey = (int)cmd;
    irCode = irCodes[irKey];

    /**
    Serial.print("Command = ");
    Serial.println(cmd, HEX); // print
    Serial.println(irCode);       // print decimal
    Serial.println(char(irCode)); // print character  
    Serial.print("Address = ");
    Serial.println(IrReceiver.decodedIRData.address, HEX);
    Serial.print("Raw data = ");
    Serial.println(IrReceiver.decodedIRData.decodedRawData, HEX);
    Serial.println();
    IrReceiver.printIRSendUsage(&Serial);
    */

    IrReceiver.resume(); // resume receiving the next key press
    }
  return rc;
  }  

// assign the ascii display character to the infrared remote control key code 
void irLoadKeyCodes()
  {
  // assign 0 to infrared remote control key codes that don't have an ascii display character  
  for (int i = 0; i <= 255; i++)
    {   
    irCodes[i] = 0;
    }  
  irCodes[KEYSPACE]=32;
  irCodes[KEYALTEXCLAMATION]=33;
  irCodes[KEYALTDOUBLEQUOTE]=34;
  irCodes[KEYALTPOUND]=35;
  irCodes[KEYALTDOLLAR]=36;
  irCodes[KEYALTPERCENT]=37;
  irCodes[KEYALTAMPERSAND]=38;
  irCodes[KEYALTSINGLEQUOTE]=39;
  irCodes[KEYALTLEFTPAREN]=40;
  irCodes[KEYALTRIGHTPAREN]=41;
  irCodes[KEYALTASTERISK]=42;
  irCodes[KEYALTPLUS]=43;
  irCodes[KEYCOMMA]=44;
  irCodes[KEYALTMINUS]=45;
  irCodes[KEYPERIOD]=46;
  irCodes[KEYDASH]=46;
  irCodes[KEYFORWARDSLASH]=47;
  irCodes[KEY0]=48;
  irCodes[KEY1]=49;
  irCodes[KEY2]=50;
  irCodes[KEY3]=51;
  irCodes[KEY4]=52;
  irCodes[KEY5]=53;
  irCodes[KEY6]=54;
  irCodes[KEY7]=55;
  irCodes[KEY8]=56;
  irCodes[KEY9]=57;
  irCodes[KEYALT0]=48;
  irCodes[KEYALT1]=49;
  irCodes[KEYALT2]=50;
  irCodes[KEYALT3]=51;
  irCodes[KEYALT4]=52;
  irCodes[KEYALT5]=53;
  irCodes[KEYALT6]=54;
  irCodes[KEYALT7]=55;
  irCodes[KEYALT8]=56;
  irCodes[KEYALT9]=57;
  irCodes[KEYCOLON]=58;
  irCodes[KEYALTSEMICOLON]=59;
  irCodes[KEYALTLESSTHAN]=60;
  irCodes[KEYALTEQUAL]=61;
  irCodes[KEYALTGREATERTHAN]=62;
  irCodes[KEYALTQUESTION]=63;
  irCodes[KEYAT]=64;
  irCodes[KEYA]=65;
  irCodes[KEYB]=66;
  irCodes[KEYC]=67;
  irCodes[KEYD]=68;
  irCodes[KEYE]=69;
  irCodes[KEYF]=70;
  irCodes[KEYG]=71;
  irCodes[KEYH]=72;
  irCodes[KEYI]=73;
  irCodes[KEYJ]=74;
  irCodes[KEYK]=75;
  irCodes[KEYL]=76;
  irCodes[KEYM]=77;
  irCodes[KEYN]=78;
  irCodes[KEYO]=79;
  irCodes[KEYP]=80;
  irCodes[KEYQ]=81;
  irCodes[KEYR]=82;
  irCodes[KEYS]=83;
  irCodes[KEYT]=84;
  irCodes[KEYU]=85;
  irCodes[KEYV]=86;
  irCodes[KEYW]=87;
  irCodes[KEYX]=88;
  irCodes[KEYY]=89;
  irCodes[KEYZ]=90;
  irCodes[KEYALTLEFTBRACKET]=91;
  irCodes[KEYALTBACKSLASH]=92;
  irCodes[KEYALTRIGHTBRACKET]=93;
  irCodes[KEYALTUNDERSCORE]=95;
  irCodes[KEYa]=97;
  irCodes[KEYb]=98;
  irCodes[KEYc]=99;
  irCodes[KEYd]=100;
  irCodes[KEYe]=101;
  irCodes[KEYf]=102;
  irCodes[KEYg]=103;
  irCodes[KEYh]=104;
  irCodes[KEYi]=105;
  irCodes[KEYj]=106;
  irCodes[KEYk]=107;
  irCodes[KEYl]=108;
  irCodes[KEYm]=109;
  irCodes[KEYn]=110;
  irCodes[KEYo]=111;
  irCodes[KEYp]=112;
  irCodes[KEYq]=113;
  irCodes[KEYr]=114;
  irCodes[KEYs]=115;
  irCodes[KEYt]=116;
  irCodes[KEYu]=117;
  irCodes[KEYv]=118;
  irCodes[KEYw]=119;
  irCodes[KEYx]=120;
  irCodes[KEYy]=121;
  irCodes[KEYz]=122;
  irCodes[KEYALTTILDE]=126;
  irCodes[KEYALTBACKSPACE]=127;
  }

// initialize the microcontrollers rgb led
void ledBegin() 
  { 
  rgbLed.begin(); 
  ledColor(RGB_OFF);
  }   

// display rgb led color for specified number of milliseconds
void ledBlink(const RGB& rgbColor,int milliSeconds)
  {  
  ledColor(rgbColor);
  delay(milliSeconds);
  ledColor(RGB_OFF);
  }

// display the specified rgb led color until it's changed by calling this function again
void ledColor(const RGB& color)
  {
  rgbLed.setPixelColor(0, rgbLed.Color(color.r, color.g, color.b));
  rgbLed.show();
  } 

void showMenu(int menuIndex,char * title, char * s1, char * s2, char * s3, char * s4, char * s5, char * s6, int waitMillis)
  {
  char t1[22]="";
  char t2[22]="";
  char t3[22]="";
  char t4[22]="";
  char t5[22]="";
  char t6[22]="";
  char spaces[22]="";
  int maxL = 0;
  int s1L = strlen(s1);
  int s2L = strlen(s2);
  int s3L = strlen(s3);
  int s4L = strlen(s4);
  int s5L = strlen(s5);
  int s6L = strlen(s6);
  if (s1L > maxL) 
    maxL = s1L;
  if (s2L > maxL) 
    maxL = s2L;
  if (s3L > maxL) 
    maxL = s3L;
  if (s4L > maxL) 
    maxL = s4L;
  if (s5L > maxL) 
    maxL = s5L;
  if (s6L > maxL) 
    maxL = s6L;
  for (int i=0;i<(11-(((maxL+3)/2)+1));i++)
    {
    spaces[i]=' ';
    spaces[i+1]=0;                 
    }
  if (s1L > 0)
    {
    if (menuIndex == 1)  
      sprintf(t1,"%s%s%s",spaces,"-> ",s1);
    else
      sprintf(t1,"%s%s%s",spaces,"   ",s1);    
    }
  if (s2L > 0)
    {
    if (menuIndex == 2)  
      sprintf(t2,"%s%s%s",spaces,"-> ",s2);
    else
      sprintf(t2,"%s%s%s",spaces,"   ",s2);    
    }
  if (s3L > 0)
    {
    if (menuIndex == 3)  
      sprintf(t3,"%s%s%s",spaces,"-> ",s3);
    else
      sprintf(t3,"%s%s%s",spaces,"   ",s3);    
    }
  if (s4L > 0)
    {
    if (menuIndex == 4)  
      sprintf(t4,"%s%s%s",spaces,"-> ",s4);
    else
      sprintf(t4,"%s%s%s",spaces,"   ",s4);    
    } 
  if (s5L > 0)
    {
    if (menuIndex == 5)  
      sprintf(t5,"%s%s%s",spaces,"-> ",s5);
    else
      sprintf(t5,"%s%s%s",spaces,"   ",s5);    
    }
  if (s6L > 0)
    {
    if (menuIndex == 6)  
      sprintf(t6,"%s%s%s",spaces,"-> ",s6);
    else
      sprintf(t6,"%s%s%s",spaces,"   ",s6);    
    }
  oledMenuMessages(title,t1,t2,t3,t4,t5,t6,waitMillis);    
  } 

int confirmMenu(char * menuTitle)
  {
  bool mainLoop = true;
  bool subLoop = true;
  confirmMenuIndex = 1; 
  while (mainLoop)
    {     
    if (confirmMenuIndex == 1)  
      oledMenuMessages(menuTitle, "", "    Are you sure?", "", "      -> 1.No", "         2.Yes", " ", 0);
    else if (confirmMenuIndex == 2)  
      oledMenuMessages(menuTitle, "", "    Are you sure?", "", "         1.No", "      -> 2.Yes", " ", 0);
    subLoop = true;            
    while (subLoop)
      {
      yield();
      if (irGet())
        {
        if ((irKey == KEYOK) || (irKey == KEYENTER))
          {
          if (confirmMenuIndex == 1)
            irKey = KEYBACK;
          else if (confirmMenuIndex == 2)
            irKey = KEYOK;  
          }      
        if (irKey == KEY1)
          {
          confirmMenuIndex = 1;
          irKey = KEYBACK;
          mainLoop = false;
          subLoop = false;
          }
        else if (irKey == KEY2)
          {
          confirmMenuIndex = 2;
          irKey = KEYOK;
          mainLoop = false;
          subLoop = false;
          }
        else if (irKey == KEYUP)
          {
          if (confirmMenuIndex-1  >= 1)
            confirmMenuIndex--;
          else
            confirmMenuIndex = 2;  
          subLoop = false;   
          }
        else if (irKey == KEYDOWN)
          {
          if (confirmMenuIndex+1  <= 2)
            confirmMenuIndex++;
          else
            confirmMenuIndex = 1;  
          subLoop = false;   
          }  
        else if ((irKey == KEYEXIT) || (irKey == KEYMENU) || (irKey == KEYOK) || (irKey == KEYBACK))
          { 
          mainLoop = false;
          subLoop = false;    
          }       
        }
      }
    }
  return irKey;  
  } 

int settingsUpdate(char * settingFilepath, char * settingName, char * settingChars, int settingSize)
  {
  bool mainLoop = true;
  bool subLoop = true;
  char newSetting[64];
  int newIndex = 0;
  newSetting[newIndex] = 0;
  char oldSetting[64];
  sprintf(oldSetting,"%s",settingChars);     
  while (mainLoop)
    {
    oledMessages(settingName, oldSetting, newSetting, "", "Specify new setting", "and then press", "OK or ENTER", 0);
    subLoop = true;
    while (subLoop)
      {
      yield();
      if (irGet())
        {
        if ((irKey == KEYEXIT) || (irKey == KEYBACK) || (irKey == KEYMENU))
          {
          mainLoop = false;  
          subLoop = false;
          }
        else if ((irKey == KEYBACKSPACE) || (irKey == KEYLEFT) || (irKey == KEYBACKLEFT))
          {
          if (newIndex > 0)
            {
            newIndex--;
            newSetting[newIndex] = 0;
            subLoop = false;
            }
          }
        else if ((irKey == KEYENTER) || (irKey == KEYOK))
          {
          if (strlen(newSetting) > 0)
            {  
            settingWrite(settingFilepath, newSetting,true);
            mainLoop = false;
            subLoop = false;
            }
          }
        else if ((irCode >= 32) && (newIndex < (settingSize-1)))
          {
          newSetting[newIndex] = irCode;
          newIndex++;
          newSetting[newIndex] = 0;
          subLoop = false;
          }         
        }
      }
    }
  return irKey;  
  }

int mainMenu()
  {
  bool mainLoop = true;
  bool subLoop = true;  
  while (mainLoop)
    {
    showMenu(mainMenuIndex,"MENU","1.Wi-Fi SSID","2.Wi-Fi Password","3.Set Clock","4.Factory Reset","","",0);
    subLoop = true;            
    while (subLoop)
      {
      yield();
      if (irGet())
        {
        if (irKey == KEYOK)
          {
          if (mainMenuIndex == 1)
            irKey = KEY1;
          else if (mainMenuIndex == 2)
            irKey = KEY2;
          else if (mainMenuIndex == 3)
            irKey = KEY3;
          else if (mainMenuIndex == 4)
            irKey = KEY4;                     
          }    
        if (irKey == KEY1)
          {
          mainMenuIndex = 1;
          irKey = settingsUpdate(wifiSSIDFilepath, "SSID", wifiSSID, sizeof(wifiSSID));
          }
        else if (irKey == KEY2)
          {
          mainMenuIndex = 2;  
          irKey = settingsUpdate(wifiPasswordFilepath, "PASSWORD", wifiPassword, sizeof(wifiPassword));
          }         
        else if (irKey == KEY3)
          {
          mainMenuIndex = 3;
          irKey = clockUpdate();
          }
        else if (irKey == KEY4)
          {
          mainMenuIndex = 4;
          irKey = confirmMenu("RESET");
          if (irKey == KEYOK)
            {
            settingsRestoreDefaults();  
            oledMessages("FACTORY", "RESET", "DONE", "", "", "", "", 3000);
            ESP.restart(); 
            }
          }                         
        else if ((irKey == KEYEXIT) || (irKey == KEYBACK) || (irKey == KEYMENU))
          {
          mainLoop = false;  
          subLoop = false;
          }
        else if (irKey == KEYUP)
          {
          if (mainMenuIndex-1  >= 1)
            mainMenuIndex--;
          else
            mainMenuIndex = 4;  
          subLoop = false;   
          }
        else if (irKey == KEYDOWN)
          {
          if (mainMenuIndex+1  <= 4)
            mainMenuIndex++;
          else
            mainMenuIndex = 1;  
          subLoop = false;   
          }  
        if ((irKey == KEYENTER) || (irKey == KEYOK) || (irKey == KEYBACK))
          subLoop = false;
        else if ((irKey == KEYEXIT) || (irKey == KEYMENU))
          { 
          mainLoop = false;
          subLoop = false;    
          }       
        }
      }
    }
  return irKey;  
  }

uint64_t clockUpdate()
  {
  bool mainLoop = true;
  bool subLoop = true;
  int workIndex = 0;
  workarea[workIndex] = 0;
  while (mainLoop)
    {
    oledMessages("SET CLOCK", workarea, "", "Specify", "ccyy-mm-dd hh:mm:ss", "and then press", "OK or ENTER", 0);
    subLoop = true;
    while (subLoop)
      {
      yield();
      if (irGet())
        {
        if ((irKey == KEYEXIT) || (irKey == KEYBACK) || (irKey == KEYMENU))
          {
          mainLoop = false;
          subLoop = false;
          }
        else if ((irKey == KEYBACKSPACE) || (irKey == KEYLEFT) || (irKey == KEYBACKLEFT))
          {
          if (workIndex > 0)
            {
            workIndex--;
            workarea[workIndex] = 0;
            subLoop = false;
            }
          }
        else if ((irKey == KEYENTER) || (irKey == KEYOK))
          {
          if (workIndex >= 19)
            { 
            clockSet(workarea);
            mainLoop = false;
            subLoop = false;
            }
          }
        else if (isDigit(irCode))
          {
          workarea[workIndex] = irCode;
          workIndex++;
          if ((workIndex == 4) || (workIndex == 7))
            {
            workarea[workIndex] = '-';
            workIndex++;  
            }
          else if (workIndex == 10)
            {
            workarea[workIndex] = ' ';
            workIndex++;  
            } 
          else if ((workIndex == 13) || (workIndex == 16))
            {
            workarea[workIndex] = ':';
            workIndex++;  
            }                       

          workarea[workIndex] = 0;
          subLoop = false;  
          }      
        }
      }
    }
  return irKey;  
  }  

// oled display initialization
void oledBegin()
  {
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
    // Serial.println("ssd1306 oled failed");
    ledColor(RGB_RED);   
    while (true);
    }
  oled.clearDisplay();       // clear buffer
  oled.display();            // display buffer
  oled.setTextColor(WHITE);  // draw in white text
  oled.cp437(true);          // use full 256 character Code Page 437 font
  }

// add a text line to the oled buffer
void oledPrint(int row, int align, int textSize, char * text)
  {
  oled.setTextSize(textSize);
  if (align == 0)           // center
    oled.setCursor(OLED_WIDTH / 2 - (((strlen(text)) * (textSize * 6)) / 2), row);
  else if (align == 1)      // left
    oled.setCursor(0, row);
  else                      // right
    oled.setCursor(OLED_WIDTH - ((strlen(text) * (textSize * 6))), row);
  oled.print(text);
  }

void oledPrint2(int row, int align, int textSize, char * text)
  {
  oled.setTextSize(textSize);
  if (align == 0)       // center
    oled.setCursor(96-(((strlen(text))*(textSize*6))/2),row);
  else if (align == 1)  // left     
    oled.setCursor(64,row);
  else                  // right
    oled.setCursor(128-((strlen(text)*(textSize*6))),row);    
  oled.print(text);
  } 

void oledMenuMessages(char * title, char * msg1, char * msg2, char * msg3, char * msg4, char * msg5, char * msg6 , int milliSeconds)
  {  
  oled.clearDisplay();
  if (strlen(msg6) > 0)
    {
    oledPrint(0, 0, 2, title);
    oledPrint(16, 1, 1, msg1);
    oledPrint(24, 1, 1, msg2);
    oledPrint(32, 1, 1, msg3);
    oledPrint(40, 1, 1, msg4);
    oledPrint(48, 1, 1, msg5);
    oledPrint(56, 1, 1, msg6);
    }
  else if (strlen(msg5) > 0)
    {
    oledPrint(0, 0, 2, title);
    oledPrint(24, 1, 1, msg1);
    oledPrint(32, 1, 1, msg2);
    oledPrint(40, 1, 1, msg3);
    oledPrint(48, 1, 1, msg4);
    oledPrint(56, 1, 1, msg5);
    }
  else if (strlen(msg4) > 0)
    {
    oledPrint(0, 0, 2, title);
    oledPrint(24, 1, 1, msg1);
    oledPrint(32, 1, 1, msg2);
    oledPrint(40, 1, 1, msg3);
    oledPrint(48, 1, 1, msg4);
    }
  else if (strlen(msg3) > 0)
    {
    oledPrint(0, 0, 2, title);
    oledPrint(24, 1, 1, msg1);
    oledPrint(40, 1, 1, msg2);
    oledPrint(56, 1, 1, msg3);
    }
  else if (strlen(msg2) > 0)
    {
    oledPrint(0, 0, 2, title);
    oledPrint(24, 1, 2, msg1);
    oledPrint(48, 1, 2, msg2);
    }
  else
    {
    oledPrint(0, 0, 2, title);
    oledPrint(24, 1, 2, msg1);
    }
  oled.display();
  if (milliSeconds > 0)
    {
    unsigned long ms = millis();
    while ((millis() - ms) < milliSeconds)
      {  
      delay(100);  
      } 
    }        
  }         

// add multiple text lines to the oled buffer and when finished display the oled buffer
void oledMessages(char * title, char * msg1, char * msg2, char * msg3, char * msg4, char * msg5, char * msg6 , int milliSeconds)
  {
  oled.clearDisplay();
  if (strlen(msg6) > 0)
    {
    oledPrint(0, 0, 2, title);
    oledPrint(16, 0, 1, msg1);
    oledPrint(24, 0, 1, msg2);
    oledPrint(32, 0, 1, msg3);
    oledPrint(40, 0, 1, msg4);
    oledPrint(48, 0, 1, msg5);
    oledPrint(56, 0, 1, msg6);
    }
  else if (strlen(msg5) > 0)
    {
    oledPrint(0, 0, 2, title);
    oledPrint(24, 0, 1, msg1);
    oledPrint(32, 0, 1, msg2);
    oledPrint(40, 0, 1, msg3);
    oledPrint(48, 0, 1, msg4);
    oledPrint(56, 0, 1, msg5);
    }
  else if (strlen(msg4) > 0)
    {
    oledPrint(0, 0, 2, title);
    oledPrint(24, 0, 1, msg1);
    oledPrint(32, 0, 1, msg2);
    oledPrint(40, 0, 1, msg3);
    oledPrint(48, 0, 1, msg4);
    }
  else if (strlen(msg3) > 0)
    {
    oledPrint(0, 0, 2, title);
    oledPrint(24, 0, 1, msg1);
    oledPrint(40, 0, 1, msg2);
    oledPrint(56, 0, 1, msg3);
    }
  else if (strlen(msg2) > 0)
    {
    oledPrint(0, 0, 2, title);
    oledPrint(24, 0, 2, msg1);
    oledPrint(48, 0, 2, msg2);
    }
  else
    {
    oledPrint(0, 0, 2, title);
    oledPrint(24, 0, 2, msg1);
    }
  oled.display();
  if (milliSeconds > 0)
    delay(milliSeconds);
  }

void oledDisplayBitmap(char * title, char * msg1, char * msg2, char * msg3, char * msg4, char * msg5, int hposition, int vposition, const unsigned char byteCodes[], int imageWidth, int imageHeight, int milliSeconds)
  {
  oled.clearDisplay();
  oled.drawBitmap(hposition,vposition,byteCodes,imageWidth,imageHeight,WHITE);
  if (strlen(msg5) > 0)
    {
    oledPrint(0,0,2,title);
    oledPrint(24,0,1,msg1);
    oledPrint(32,0,1,msg2);
    oledPrint(40,0,1,msg3);
    oledPrint(48,0,1,msg4);
    oledPrint(56,0,1,msg5);     
    }
  else if (strlen(msg4) > 0)
    {
    oledPrint(0,0,2,title);
    oledPrint(24,0,1,msg1);
    oledPrint(34,0,1,msg2);
    oledPrint(42,0,1,msg3);
    oledPrint(50,0,1,msg4);
    }
  else if (strlen(msg3) > 0)
    {
    oledPrint(0,0,2,title);
    oledPrint2(24,0,1,msg1);
    oledPrint2(40,0,1,msg2);
    oledPrint2(56,0,1,msg3);        
    }
  else if (strlen(msg2) > 0)
    {
    oledPrint(0,0,2,title);
    oledPrint2(24,0,2,msg1);
    oledPrint2(48,0,2,msg2);       
    }
  else
    {
    oledPrint(0,0,2,title);  
    oledPrint2(36,0,2,msg1);    
    } 
  oled.display();
  if (milliSeconds > 0)
    {
    unsigned long ms = millis();
    while ((millis() - ms) < milliSeconds)
      { 
      delay(100); 
      }    
    }     
  }

// copy a subset of characters from one character array to another character array
void copyChars(char * inBuf, int startPos, int copyCount, char * outBuf)
  {
  int outPos = 0;
  while (copyCount > 0)
    {
    outBuf[outPos] = inBuf[startPos];
    startPos++;
    outPos++;
    copyCount--;
    }
  outBuf[outPos] = 0;
  }

// prevent too many .csv filenames and too many .csv records exceeding our xml buffer 
void catXml(char * items, char * item)
  {
  if ((strlen(items)+strlen(item)) < (ITEMS_SIZE-sizeof("</Data>\n")))
    strcat(items,item);  
  }  

// our web server sends this xml response to the web browser xml request 
void sendXml(AsyncWebServerRequest *request, char * requestId, char * message)
    {  
    strcpy(xmlItems, "<?xml version='1.0'?>\n");
    catXml(xmlItems, "<Data>\n");

    sprintf(xmlItem, "<requestId>%s</requestId>", requestId);
    catXml(xmlItems, xmlItem);

    sprintf(xmlItem, "<relayModeId>%s</relayModeId>", relayMode);
    catXml(xmlItems, xmlItem);

    sprintf(xmlItem, "<relayStateId>%s</relayStateId>", relayState);
    catXml(xmlItems, xmlItem);                               

    sprintf(xmlItem, "<messageId>%s</messageId>", message);
    strcat(xmlItems, xmlItem);

    sprintf(xmlItem, "<temperatureId>%s%s</temperatureId>", temperatureChar,"\xC2\xB0");
    catXml(xmlItems, xmlItem);

    sprintf(xmlItem, "<humidityId>%s%s</humidityId>", humidityChar,"%");
    catXml(xmlItems, xmlItem);

    if (strcmp(requestId,"refreshReport") == 0)
      {
      // encloses nested (embedded) xml using <![CDATA[embedded xml goes here]]>
      // reportFilesnames contains something like <option selected>2026-07-15.csv</option><option>2026-07-14.csv</option>        
      sprintf(xmlItem, "<reportFilenamesId><![CDATA[%s]]></reportFilenamesId>", reportFilenames);    
      catXml(xmlItems, xmlItem);

      // encloses nested (embedded) xml using <![CDATA[embedded xml goes here]]>
      // reportData contains something like <tr><td>00:26</td><td>82</td><td>46</td></tr><tr><td>00:30</td><td>83</td><td>46</td></tr>
      sprintf(xmlItem, "<reportDataId><![CDATA[%s]]></reportDataId>", reportData);
      catXml(xmlItems, xmlItem);
      }

    catXml(xmlItems, "</Data>\n");
    request->send_P(200, "text/xml", xmlItems);
    ledBlink(RGB_BLUE,50);
    } 

// retrieve each setting stored as the first and only record in its own plain text file
void settingsLoad()
  {
  settingRead(relayModeFilepath, relayMode, relayModeDefault,sizeof(relayMode));    
  settingRead(calibrateTemperatureFilepath, calibrateTemperature, calibrateTemperatureDefault,sizeof(calibrateTemperature));
  temperatureCalibration = atoi(calibrateTemperature);
  settingRead(calibrateHumidityFilepath, calibrateHumidity, calibrateHumidityDefault,sizeof(calibrateHumidity)); 
  humidityCalibration = atoi(calibrateHumidity); 
  settingRead(wifiSSIDFilepath,wifiSSID,wifiSSIDDefault,sizeof(wifiSSID));
  settingRead(wifiPasswordFilepath,wifiPassword,wifiPasswordDefault,sizeof(wifiPassword));
  }

// restore all the original configuration settings
void settingsRestoreDefaults()
  {
  settingWrite(relayModeFilepath,relayModeDefault,false);
  settingWrite(calibrateTemperatureFilepath,calibrateTemperatureDefault,false);
  temperatureCalibration = atoi(calibrateTemperature);
  settingWrite(calibrateHumidityFilepath,calibrateHumidityDefault,false); 
  humidityCalibration = atoi(calibrateHumidity); 
  settingWrite(wifiSSIDFilepath,wifiSSIDDefault,false);
  settingWrite(wifiPasswordFilepath,wifiPasswordDefault,false);
  relayOff();
  settingsLoad();
  }   

void settingRead(char * filepath, char * setting, char * defaultSetting, int settingSize)
  {
  byte oneByte;
  setting[0]=0;
  int offset=0; 
  if (!SD.exists(filepath))
    {
    File fOut = SD.open(filepath, FILE_WRITE);
    if (fOut)
      {  
      fOut.println(defaultSetting);
      fOut.close();  
      }
    }
  // read the setting file that may or may not have been manually edited by the user  
  File fIn = SD.open(filepath, FILE_READ);
  if (fIn)
    {
    while (fIn.available())
      {
      oneByte = fIn.read();
      if ((oneByte != 0x0D) && (oneByte != 0x0A))
        {
        // ignore trailing setting characters that exceed the setting character array  
        if (offset < (settingSize-1))
          {
          setting[offset]=oneByte;
          offset++;
          setting[offset] = 0;
          }
        }
      else
        {
        break;  
        }          
      }           
    fIn.close();
    }
  // move null terminator to eliminate any trailing blanks
  for (;offset > 0;offset--) 
    {
    // while there are trailing blanks, replace them with string terminating null character
    if (setting[offset-1] == 0x20)
      setting[offset-1] = 0;
    // else it's not a space character so we are done trimming trailing blanks
    else
      break;   
    } 
  }

// overwrite specified setting file
void settingWrite(char * filepath, char * setting, bool reload)
  {
  File fOut = SD.open(filepath, FILE_WRITE);
  if (fOut)
    {  
    fOut.println(setting);  // record ends with \r\n being 0x0D and 0x0A 
    fOut.close();  
    }
  if (reload)
    settingsLoad();    
  }

// set relay off or on based on current temperature  
void relayModeCheck()
  {
  if (strcmp(relayMode,"1") == 0)
    relayOff();
  else if (strcmp(relayMode,"2") == 0)
    relayOn();
  else if (strcmp(relayMode,"3") == 0)
    {
    if (temperature >= 60)
      relayOn();
    else
      relayOff();  
    }
  else if (strcmp(relayMode,"4") == 0)
    {
    if (temperature >= 65)
      relayOn();
    else
      relayOff();  
    }
  else if (strcmp(relayMode,"5") == 0)
    {
    if (temperature >= 70)
      relayOn();
    else
      relayOff();  
    }
  else if (strcmp(relayMode,"6") == 0)
    {
    if (temperature >= 75)
      relayOn();
    else
      relayOff();  
    }
  else if (strcmp(relayMode,"7") == 0)
    {
    if (temperature >= 80)
      relayOn();
    else
      relayOff();  
    }
  else if (strcmp(relayMode,"8") == 0)
    {
    if (temperature >= 85)
      relayOn();
    else
      relayOff();  
    }
  else if (strcmp(relayMode,"9") == 0)
    {
    if (temperature >= 90)
      relayOn();
    else
      relayOff();  
    }

  else if (strcmp(relayMode,"10") == 0)
    {
    if (temperature <= 60)
      relayOn();
    else
      relayOff();  
    }
  else if (strcmp(relayMode,"11") == 0)
    {
    if (temperature <= 65)
      relayOn();
    else
      relayOff();  
    }
  else if (strcmp(relayMode,"12") == 0)
    {
    if (temperature <= 70)
      relayOn();
    else
      relayOff();  
    }
  else if (strcmp(relayMode,"13") == 0)
    {
    if (temperature <= 75)
      relayOn();
    else
      relayOff();  
    }
  else if (strcmp(relayMode,"14") == 0)
    {
    if (temperature <= 80)
      relayOn();
    else
      relayOff();  
    }
  else if (strcmp(relayMode,"15") == 0)
    {
    if (temperature <= 85)
      relayOn();
    else
      relayOff();  
    }
  else
    {
    if (temperature <= 90)
      relayOn();
    else
      relayOff();  
    }
  }  

void relayOff()
  {
  strcpy(relayState,"Relay is Off");   
  digitalWrite(RELAY_PIN,HIGH);
  }  

void relayOn()
  {
  strcpy(relayState,"Relay is On");  
  digitalWrite(RELAY_PIN,LOW);
  }  

// wait for user to press any infrared remote control key
int waitForUser()
  {
  bool mainLoop = true;
  while (mainLoop)
    {
    yield();
    if (irGet())
      {
      if (!((irKey == KEYEXIT) || (irKey == KEYBACK) || (irKey == KEYMENU)))
        irKey = KEYBACK;
      mainLoop = false;  
      }
    } 
  return irKey;         
  }

void reportFilenameUpdate(char * filename)
  {
  strcpy(reportFilename,filename);
  sprintf(reportFilepath,"%s%s","/",reportFilename); 
  refreshRequested = true;
  }  

void reportDataUpdate()
  {   
  char csvField[24];
  int csvOffset = 0;  
  char tableData[256];  
  bool newTableRow = true;
  char oneByte; 
  int rowCount = 0; 
  reportData[0]=0;
  File fIn = SD.open(reportFilepath, FILE_READ);
  if (fIn)
    {         
    while (fIn.available())
      {
      oneByte = fIn.read();
      // 0x2C and 0x0D are column separators
      if ((oneByte == 0x2C) || (oneByte == 0x0D))
        {
        if (newTableRow)
          {
          // don't write the heading row  
          if (rowCount > 0)  
            strcat(reportData,"<tr>"); // start of html table row 
          newTableRow = false;  
          }
        // don't write the heading row  
        if (rowCount > 0)
          {    
          sprintf(tableData,"%s%s%s","<td>",csvField,"</td>");  
          strcat(reportData,tableData);
          }
        csvOffset = 0;
        csvField[csvOffset] = 0;  // append string ending null character
        }
      // 0x0A is last character at end of the csv record  
      else if (oneByte == 0x0A)
        {
        // don't write the heading row  
        if (rowCount > 0)
          strcat(reportData,"</tr>"); // end of html table row     
        newTableRow = true;
        rowCount++;
        }
      // else this is a byte of a csv field data   
      else
        {
        csvField[csvOffset]=oneByte;
        csvOffset++;
        csvField[csvOffset] = 0;  // append string ending null character  
        }  
      }           
    fIn.close();   
    }
  }

void reportFilenamesUpdate()
  {
  std::vector<std::string> vectorFilenames;
  char filename[] = "ccyy-mm-dd.csv";
  char optionStatement[100];

  // optimize vector performance by estimating number of vector entries per year
  vectorFilenames.reserve(366); 

  // initialize the vector to empty  
  vectorFilenames.clear();

  // add all the ccyy-mm-dd.csv filenames to a vector and then sort the vector in descending order 
  File rootDirectory = SD.open("/");
  if (rootDirectory)
    {  
    File dirEntry = rootDirectory.openNextFile();
    while (dirEntry)
      {
      if (!dirEntry.isDirectory())
        {
        stringFilename = dirEntry.name();                      
        if (stringFilename.endsWith(".csv"))
          {
          stringFilename.toCharArray(filename,sizeof(filename));
          vectorFilenames.push_back(filename);
          }
        }
      dirEntry = rootDirectory.openNextFile();
      }
    rootDirectory.close(); 
    }

  // sort the vector filenames into descending order from most recent to oldest
  std::sort(vectorFilenames.begin(), vectorFilenames.end(),std::greater<>());

  reportFilenames[0]=0;
  for(int i = 0; i < vectorFilenames.size(); i++)
    {
    // if the vector filename matches the report filename requested, show it as the item selected
    if (strcmp(vectorFilenames[i].c_str(),reportFilename) == 0)
      {
      sprintf(optionStatement, "%s%s%s", "<option selected>", vectorFilenames[i].c_str(), "</option>");       
      }  
    // else the file name in the directory does not match the file being displayed
    else
      {
      sprintf(optionStatement, "%s%s%s", "<option>", vectorFilenames[i].c_str(), "</option>");
      }        
    strcat(reportFilenames,optionStatement);    
    }     
  }          

// create the html web page 
void webpageCreate()
  {   
  webpage[0]=0;
  webpageAppend(webpageTop,"","");
  webpageAppend("<h1>Temperature Report</h1>","","");
  webpageAppend("<select id=\"reportFilenamesId\" name=\"reportFilenamesId\" class=\"reportFilenames\" onchange=\"selectReport(this)\"></select>","","");
  webpageAppend("<br>","","");
  webpageAppend("<br>","","");         
  webpageAppend("<div class=\"reportTableContainer\">","","");
  webpageAppend("<table id=\"reportTableId\" name=\"reportTableId\" style=height:100%;>","",""); 
  webpageAppend("<thead><tr><th style=\"width:24%;\"onclick=\"scrollTime()\">Time</th><th style=\"width=42%;\"onclick=\"scrollTemperature()\">Temperature</th><th style=\"width:34%;\"onclick=\"scrollHumidity()\">Humidity</th></tr></thead>","","");      
  webpageAppend("<tbody id=\"reportDataId\" name=\"reportDataId\"></tbody>","","");
  webpageAppend("</table>","","");
  webpageAppend("</div>","","");
  webpageAppend("<p id=\"messageId\" name=\"messageId\" class=\"message\">Loaded temperature report</p>","","");
  webpageAppend("<br>","","");  
  webpageAppend("<select id=\"relayModeId\" name=\"relayModeId\" class=\"relayMode\" onchange=\"relayModeChange(this.value)\">","","");
  webpageAppend("<option value=\"1\" selected>Relay Off</option>","","");
  webpageAppend("<option value=\"2\">Relay On</option>","","");
  webpageAppend("<option value=\"3\">Relay On Temperature &ge; 60</option>","","");
  webpageAppend("<option value=\"4\">Relay On Temperature &ge; 65</option>","",""); 
  webpageAppend("<option value=\"5\">Relay On Temperature &ge; 70</option>","","");
  webpageAppend("<option value=\"6\">Relay On Temperature &ge; 75</option>","","");
  webpageAppend("<option value=\"7\">Relay On Temperature &ge; 80</option>","","");
  webpageAppend("<option value=\"8\">Relay On Temperature &ge; 85</option>","","");
  webpageAppend("<option value=\"9\">Relay On Temperature &ge; 90</option>","","");
  webpageAppend("<option value=\"10\">Relay On Temperature &le; 60</option>","","");
  webpageAppend("<option value=\"11\">Relay On Temperature &le; 65</option>","",""); 
  webpageAppend("<option value=\"12\">Relay On Temperature &le; 70</option>","","");
  webpageAppend("<option value=\"13\">Relay On Temperature &le; 75</option>","","");
  webpageAppend("<option value=\"14\">Relay On Temperature &le; 80</option>","","");
  webpageAppend("<option value=\"15\">Relay On Temperature &le; 85</option>","","");
  webpageAppend("<option value=\"16\">Relay On Temperature &le; 90</option>","","");  
  webpageAppend("</select>","","");
  webpageAppend("<br>","","");
  webpageAppend("<br>","","");
  webpageAppend("<button id=\"downloadReportId\" onclick=\"downloadReport()\">Download Report</button>","","");    
  webpageAppend("<br>","","");
  webpageAppend("<br>","","");
  webpageAppend("<button onclick=\"deleteReport()\">Delete Report</button>","",""); 
  webpageAppend("<br>","","");
  webpageAppend("<br>","","");
  webpageAppend("<button id=\"calibrateTemperatureId\" onclick=\"calibrateTemperature()\">Calibrate Temperature</button>","","");    
  webpageAppend("<br>","","");
  webpageAppend("<br>","","");
  webpageAppend("<button id=\"calibrateHumidityId\" onclick=\"calibrateHumidity()\">Calibrate Humidity</button>","","");
  webpageAppend("<br>","","");
  webpageAppend("<br>","","");
  webpageAppend("<script>","","");
  webpageAppend(globalVariables,"","");  
  webpageAppend(unsolicitedEventsHandler,"","");
  webpageAppend(displayTemperatureAndHumidityFunction,"","");
  webpageAppend(xmlRequestFunction,"","");   
  webpageAppend(xmlResponseFunction,"",""); 
  webpageAppend(updateReportTableFunction,"","");    
  webpageAppend(getReportFilenameFunction,"","");    
  webpageAppend(scrollTimeFunction,"","");
  webpageAppend(scrollTemperatureFunction,"","");
  webpageAppend(scrollHumidityFunction,"","");  
  webpageAppend(relayModeChangeFunction,"","");    
  webpageAppend(selectReportFunction,"","");
  webpageAppend(refreshReportFunction,"","");
  webpageAppend(deleteReportFunction,"","");
  webpageAppend(downloadReportFunction,"","");    
  webpageAppend(calibrateTemperatureFunction,"","");      
  webpageAppend(calibrateHumidityFunction,"","");
  webpageAppend("</script>","","");
  webpageAppend(webpageBottom,"","");
  ledBlink(RGB_BLUE,50);
  }

// while copying input html characters to output buffer, find and replace matching character string
void webpageAppend(const char *html, char *find, char *replace)
  {
  int findLength = strlen(find);
  int replaceLength = strlen(replace);  
  char* webpageWorkareaPtr = webpageWorkarea;
  webpageWorkarea[0]=0; 
  if (findLength > 0)
    {
    // stay in while loop until the input character array null character is encountered
    while (*html)
      {
      // if find characters are found
      if (strstr(html, find) == html)
        {
        // copy the replacement characters to the buffer
        strcpy(webpageWorkareaPtr, replace);
        webpageWorkareaPtr += replaceLength;
        // skip past the matching characters
        html += findLength; 
        }
      // else copy the current character to the buffer   
      else
        {
        *webpageWorkareaPtr++ = *html++;
        }
      }
    // null terminate the output buffer character array  
    *webpageWorkareaPtr = 0;
    strcat(webpage,webpageWorkarea);
    }
  else
    {
    strcat(webpage,html);
    }    
  }  

// Information on how to generating QR Code
// 
// I used the following procedure.
// Used web browswer to https://www.qrcode-monkey.com to create
// a QR code for http://mywebserver.local 
// and saved it as a .png file downloaded to my pc.
// Then used Corel PaintShop Pro 2023 menu bar Image->Decrease color depth->2 color palette
// Then used Corel PaintShop Pro 2023 menu bar Image->Resize->48x48pixels          
// Then used Corel PaintShop Pro to create an empty 64x48 image
// Then copy and pasted the 48x48 qrcode image into the 64x48 image
// Save the merged image as qrcode.bmp
// Used my web browswer to access image2cpp website to read qrcode.bmp
// to generate the bytecode statements that I could cut and paste  
// into my arduino sketch as a constant and passed it as parameter
// into my oledDisplayBitmap function.
//
// Information on how to prepare and perform an over-the-air update
//
// Use Sketch->Export Compiled Binary located on the Arduino IDE Action Bar
// to compile your updated sketch and create its firmware file named something like
// yoursketchname\build\esp32.esp32.esp32c6\yoursketchname.ino.bin
// Copy yoursketchname.ino.bin file to a file directory
// on your mobile device that you can access later when
// you are ready to initiate the over-the-air update using 
// its web browser app.
// 
// When you are ready to initiate the over-the-air update,
// make sure your mobile device and your ESP32 are logged on to the same wifi network.
// Start your mobile device's web browser and navigate to http://mywebserver.local/update
// that runs as part of your existing installed and running sketch that
// supports over-the-air support similar to that shown in this demo10 sketch.
//
// The over-the-air update web server running as part of your ESP32 sketch sends your web browser
// the following display:
//
// Firmware:
// [Choose File]   No file chosen  [Update Firmware]
//
// FileSystem
// [Choose File]   No file chosen  [Update FileSystem]
//
// At this point, select the Firmware [Choose File] button.
// Navigate to yoursketchname.ino.bin file and select it.
// Then select the Firmware [Update Firmware] button
// After about 15 seconds, the following message displays
// Update Success! Rebooting...
//
// At this point, your ESP32 is running your newly uploaded sketch firmware.                     

// these statements show my software development environment
// Microsoft Windows 11 
// Arduino IDE 2.3.6
// Board: ESP32C6 Dev Module (specifically Espressif Systems ESP32-C6-DEVKITC-1-N8 Microcontroller)
// Using library WiFi at version 3.3.0 in folder: C:\Users\larry\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.0-alpha1\libraries\WiFi 
// Using library Networking at version 3.3.0 in folder: C:\Users\larry\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.0-alpha1\libraries\Network 
// Using library HTTPClient at version 3.3.0 in folder: C:\Users\larry\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.0-alpha1\libraries\HTTPClient 
// Using library NetworkClientSecure at version 3.3.0 in folder: C:\Users\larry\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.0-alpha1\libraries\NetworkClientSecure 
// Using library Async TCP at version 3.4.0 in folder: C:\Users\larry\OneDrive\Documents\Arduino\libraries\Async_TCP 
// Using library ESP Async WebServer at version 3.7.7 in folder: C:\Users\larry\OneDrive\Documents\Arduino\libraries\ESP_Async_WebServer 
// Using library FS at version 3.3.0 in folder: C:\Users\larry\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.0-alpha1\libraries\FS 
// Using library ESPmDNS at version 3.3.0 in folder: C:\Users\larry\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.0-alpha1\libraries\ESPmDNS 
// Using library ESPAsyncHTTPUpdateServer at version 3.0.0 in folder: C:\Users\larry\OneDrive\Documents\Arduino\libraries\ESPAsyncHTTPUpdateServer 
// Using library Wire at version 3.3.0 in folder: C:\Users\larry\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.0-alpha1\libraries\Wire 
// Using library Adafruit GFX Library at version 1.12.1 in folder: C:\Users\larry\OneDrive\Documents\Arduino\libraries\Adafruit_GFX_Library 
// Using library Adafruit BusIO at version 1.17.1 in folder: C:\Users\larry\OneDrive\Documents\Arduino\libraries\Adafruit_BusIO 
// Using library SPI at version 3.3.0 in folder: C:\Users\larry\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.0-alpha1\libraries\SPI 
// Using library Adafruit SSD1306 at version 2.5.13 in folder: C:\Users\larry\OneDrive\Documents\Arduino\libraries\Adafruit_SSD1306 
// Using library RTClib at version 2.0.2 in folder: C:\Users\larry\OneDrive\Documents\Arduino\libraries\RTClib 
// Using library Adafruit Unified Sensor at version 1.1.15 in folder: C:\Users\larry\OneDrive\Documents\Arduino\libraries\Adafruit_Unified_Sensor 
// Using library Adafruit BME280 Library at version 2.3.0 in folder: C:\Users\larry\OneDrive\Documents\Arduino\libraries\Adafruit_BME280_Library 
// Using library SD at version 3.3.0 in folder: C:\Users\larry\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.0-alpha1\libraries\SD 
// Using library Adafruit NeoPixel at version 1.12.5 in folder: C:\Users\larry\OneDrive\Documents\Arduino\libraries\Adafruit_NeoPixel 
// Using library ESP Mail Client at version 3.4.24 in folder: C:\Users\larry\OneDrive\Documents\Arduino\libraries\ESP_Mail_Client 
// Using library LittleFS at version 3.3.0 in folder: C:\Users\larry\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.0-alpha1\libraries\LittleFS 
// Using library Update at version 3.3.0 in folder: C:\Users\larry\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.0-alpha1\libraries\Update 
// Using library IRremote at version 4.5.0 in folder: C:\Users\larry\OneDrive\Documents\Arduino\libraries\IRremote 
// Using library Ticker at version 3.3.0 in folder: C:\Users\larry\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.0-alpha1\libraries\Ticker 
// Using library SPIFFS at version 3.3.0 in folder: C:\Users\larry\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.0-alpha1\libraries\SPIFFS 
// "C:\\Users\\larry\\AppData\\Local\\Arduino15\\packages\\esp32\\tools\\esp-rv32\\2411/bin/riscv32-esp-elf-size" -A "C:\\Users\\larry\\AppData\\Local\\arduino\\sketches\\068F73F88D27F3A9CEB6C6714E732CD3/demo10.ino.elf"
// Sketch uses 1524471 bytes (45%) of program storage space. Maximum is 3342336 bytes.
// Global variables use 192532 bytes (58%) of dynamic memory, leaving 135148 by
