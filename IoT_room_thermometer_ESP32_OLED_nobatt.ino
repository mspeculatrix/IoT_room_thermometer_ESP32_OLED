/*  IoT_room_thermometer_ESP32_OLED

	Version of the main program for a device that will be powered by 5V from
	a USB power supply. All battery-related stuff has been removed.

    ESP32-based IoT room thermometer with InfluxDB client & OLED display.

    Machina Speculatrix :: https://mansfield-devine.com/speculatrix/

    Open source - have at it!

    This code comes with no guarantees or promises.
    Don't blame me if it burns your house down.

    For a TTGO ESP32 dev board with 128x64 OLED display.

    Uses a GY-21 (SHT21/HTU21) I2C temperature & humidity sensor module.

    Because of the dev board's unusual I2C pinout – used for the OLED display - we need
    a TwoWire instance with custom pin assignments to pass to the Adafruit_SSD1306
    instance.

    For the same reason, the Environment library in the Arduino IDE doesn't work 
	for use wth the GY-21 sensor board.
    Neither did the SHT21 library. So I modified the latter to have a constructor
    that allows me to pass the same TwoWire instance that I created for the OLED display.
    This version is called SHT21_TTGO.

    Display reset pin is 16.
    Display I2C address is 0x3C (normally 0x3D for a 128x64 OLED).

    The code uses lots of Serial.print() statements for debugging purposes.
    These can be - and probably should be - removed or commented out.

    Board: ESP32 Dev Module

    NOTE: Wifi credentials and InfluxDB settings (including credentials) are stored in
    header files in my Arduino libraries folder.

    In my setup, I have two wifi APs, which is why you see the connection routine alternate
    between the two. The WifiCfgHome.h header file looks something like this:

    #define WIFI_SSID_MAIN "ssid1"
    #define WIFI_SSID_ALT "ssid2"
    #define WIFI_PASSWORD "password"

    You could always use the same setup and just make both SSIDs the same, if you have
    only one AP (poor you!).

    The InfluxDbConfig.h looks a bit like this:

    #define INFLUXDB_URL "http://10.0.30.50:8086"
    #define INFLUXDB_DB_NAME "iot"
    #define INFLUXDB_USER "iot"
    #define INFLUXDB_PASSWORD "password"

*/

// ---------------------------------------------------------------------------------------
// --- PROJECT SETTINGS                                                                ---
// ---------------------------------------------------------------------------------------
#define DEVICE_UID          "AMBTMP-WI-ESP32-0003"  // anything you like
#define DEVICE_LOCATION     "dining_room"           //    "      "   "
#define INFLUX_MEASUREMENT  "temperature"           //    "      "   "
#define INFLUX_ALERT        "alerts"
#define WIFI_MAX_TRIES      12      // No. times to try to connect before giving up
#define INFLUX_MAX_TRIES    3       // No. times to try to connect before giving up
#define LOOP_DELAY          10000   // Pause each time in the loop, in ms.
#define REPORT_INTERVAL     30      // No. times through loop before reporting to InfluxDB
#define SDA                 4       // Pin for I2C SDA
#define SCL                 15      // Pin for I2C SCL          
#define DISPLAY_ADDR        0x3C    // I2C address for OLED display
#define SENSOR_ADDR         0x40    // I2C address for SHT21 sensor board
#define LINE_HEIGHT_1       8       // Height of font size 1 in pixels
#define HUM_Y               30
#define ALIGN_LEFT          0
#define ALIGN_RIGHT         1
// ---------------------------------------------------------------------------------------

#include <WiFiMulti.h>
#define DEVICE "ESP32"

// SHT21 temp/humidity module - modified library
#include <SHT21_TTGO.h>

// OLED display
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#define SCREEN_WIDTH  128         // OLED display width, in pixels
#define SCREEN_HEIGHT  64         // OLED display height, in pixels
#define OLED_RESET     16         // Reset pin - changed to 16 for TTGO board
#define TEMPC_X_POS 1
#define TEMPC_Y_POS 36

// Wifi credentials. Sets: WIFI_SSID_MAIN, WIFI_SSID_ALT, WIFI_PASSWORD
#include <WifiCfgHome.h>

#include <InfluxDbClient.h>
// Include InfluxDB settings.
// Sets: INFLUXDB_URL, INFLUXDB_DB_NAME, INFLUXDB_USER, INFLUXDB_PASSWORD
#include <InfluxDbConfig.h>

// Function prototypes
void printIP();
void printLine(String text, uint8_t line, uint8_t align, uint8_t adjusty);
void printTemperature(const float temp);
void printHumidity(const float hum);
bool checkSHT21();
bool wifiConnect();

// ---------------------------------------------------------------------------------------
// --- GLOBALS                                                                         ---
// ---------------------------------------------------------------------------------------

bool wifiConnected;
bool influxConnected;

float temperature;
float humidity;

// ---------------------------------------------------------------------------------------
// --- MAIN OBJECTS                                                                    ---
// ---------------------------------------------------------------------------------------
// Wifi client
WiFiMulti wifiMulti;

// InfluxDB
InfluxDBClient dbclient(INFLUXDB_URL, INFLUXDB_DB_NAME);
Point sensor(INFLUX_MEASUREMENT);

// I2C
TwoWire twi = TwoWire(1); // create our own TwoWire instance

// Display - uses TwoWire instance
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &twi, OLED_RESET);

// SHT21 module  - uses TwoWire instance
SHT21_TTGO sht = SHT21_TTGO(&twi);

// ---------------------------------------------------------------------------------------
// --- NETWORK                                                                         ---
// ---------------------------------------------------------------------------------------
const char* ssid [] = { WIFI_SSID_MAIN, WIFI_SSID_ALT };
IPAddress ip;

// ---------------------------------------------------------------------------------------
// --- DISPLAY FUNCTIONS                                                               ---
// ---------------------------------------------------------------------------------------

void printIP() {
  /*  Prints the IP address in the bottom-right corner of the display */
  printLine(ip.toString(), 7, ALIGN_RIGHT, -4);
}

void printLine(String text, uint8_t line, uint8_t align = ALIGN_LEFT, uint8_t adjusty = 0) {
  /* Prints text using the OLED displays default font, size 1 */
  display.setFont();        // don't use GFX fonts
  display.setTextSize(1);
  uint8_t ypos = ((line - 1) * LINE_HEIGHT_1) + adjusty;
  display.fillRect(0, ypos, SCREEN_WIDTH, LINE_HEIGHT_1, BLACK);
  display.setCursor(0, ypos);
  if(align == ALIGN_RIGHT) {
    char buf[text.length() + 1];
    sprintf(buf, "%s", text.c_str());
    int16_t  x1, y1; uint16_t w, h;
    display.getTextBounds(buf, 1, 1, &x1, &y1, &w, &h);
    display.setCursor(SCREEN_WIDTH - w, ypos);
  }
  display.print(text);
  display.display();
}

void printTemperature(const float temp) {
  /* Display temperature on the OLED display */
  char buf[10];
  display.setFont(&FreeSans24pt7b);
  display.fillRect(1, 1, SCREEN_WIDTH, 40, BLACK);  // erase previous reading
  display.setCursor(TEMPC_X_POS, TEMPC_Y_POS);      // origin of text
  sprintf(buf, "%.1f", temp);
  // get size & location of new text, for use by degree symbol below
  int16_t  x1, y1; uint16_t w, h;
  display.getTextBounds(buf, TEMPC_X_POS, TEMPC_Y_POS, &x1, &y1, &w, &h);
  display.print(buf);

  // print degree symbol & 'C'
  display.setFont(&FreeSans9pt7b);
  display.setCursor(x1 + w + 2, 13);
  display.print("o");                                                                                         
  display.setFont(&FreeSans12pt7b);
  display.setCursor(x1 + w + 12, 22);
  display.print("C");
  Serial.print("Temp: "); Serial.println(buf);
}

void printHumidity(const float hum) {
  /* Show the humidity on the OLED display */
  display.setFont();        // don't use GFX fonts
  display.setTextSize(1);
  display.fillRect(96, HUM_Y, 32, LINE_HEIGHT_1, BLACK);  // erase previous reading
  char buf[7];
  sprintf(buf, "%.0f%%", hum);
  int16_t  x1, y1; uint16_t w, h;
  display.getTextBounds(buf, 1, 1, &x1, &y1, &w, &h);
  display.setCursor(SCREEN_WIDTH - w, HUM_Y);
  display.print(buf);
}

// ---------------------------------------------------------------------------------------
// --- SENSOR FUNCTIONS                                                                ---
// ---------------------------------------------------------------------------------------
bool checkSHT21() {
  /* Check for the presence of the SHT21 sensor board.
     We use the return value of the Write.endTransmission() method to see if
     a device acknowledges the address. */
  bool found = false;
  Serial.println("Checking for SHT21 sensor...");
  twi.beginTransmission(SENSOR_ADDR);
  byte result = twi.endTransmission();
  Serial.print("Result: "); Serial.println(result);
  switch (result) {
    case 0: // success
    case 1: // data too long for transmit buffer, but there is a device
      Serial.print("Sensor found at address: ");
      found = true;
      break;
    case 4:
      Serial.print("Unknown error at address: ");
      break;
    default:
      Serial.print("Sensor NOT FOUND at address: ");
      break;
  }
  Serial.print("0x"); Serial.println(SENSOR_ADDR, HEX);
  return found;
}

// ---------------------------------------------------------------------------------------
// --- WIFI FUNCTIONS                                                                  ---
// ---------------------------------------------------------------------------------------
bool wifiConnect() {
  display.clearDisplay();
  bool connected = false;
  uint8_t ssid_idx = 0;
  uint8_t connect_counter = 0;
  WiFi.mode(WIFI_STA);
  Serial.println("Connecting to wifi...");
  printLine("Connecting to wifi:", 1);
  while (connect_counter < WIFI_MAX_TRIES) {
    WiFi.mode(WIFI_STA);
    Serial.print("Trying "); Serial.println(ssid[ssid_idx]);
    printLine(ssid[ssid_idx], 2);
    wifiMulti.addAP(ssid[ssid_idx], WIFI_PASSWORD);
    delay(2000);
    if (wifiMulti.run() != WL_CONNECTED) {
      ssid_idx = 1 - ssid_idx;    // swap APs
    } else {
      connect_counter = WIFI_MAX_TRIES;
      ip = WiFi.localIP();
      Serial.print("Connected: "); Serial.println(ip);
      connected = true;
    }
    connect_counter++;
  }
  return connected;
}

// ***************************************************************************************
// ***  SETUP                                                                          ***
// ***************************************************************************************
void setup() {

  wifiConnected = false;
  influxConnected = false;

  Serial.begin(57600);
  Serial.println(); Serial.println(); Serial.println("Setting up...");
  Serial.print("Device ID: "); Serial.println(DEVICE_UID);
  Serial.print("Location : "); Serial.println(DEVICE_LOCATION);

  Serial.println("Starting display...");
  twi.begin(SDA, SCL); // Needs to come before display.begin() is used
  if (!display.begin(SSD1306_SWITCHCAPVCC, DISPLAY_ADDR)) {
    Serial.println(F("SSD1306 allocation failed. Aborting."));
    while (1); // Don't proceed, loop forever
  }
  // Clear the display buffer
  display.clearDisplay();
  display.setTextColor(WHITE); // or BLACK);

  if (!checkSHT21()) {
    Serial.println(F("Sensor not found. Aborting."));
    printLine("*** FAILED ***", 1);
    printLine("Sensor not found.", 2);
    printLine("Aborting.", 3);
    while (1); // Don't proceed, loop forever
  }

  wifiConnected = wifiConnect();

  if (wifiConnected) {
    printLine("Wifi connected", 3);
    Serial.println(F("Setting up InfluxDB connection"));
    Serial.print(F("Server  : ")); Serial.println(INFLUXDB_URL);
    Serial.print(F("Database: ")); Serial.println(INFLUXDB_DB_NAME);
    Serial.print(F("User    : ")); Serial.println(INFLUXDB_USER);
    uint8_t connect_counter = 0;
    while (connect_counter < INFLUX_MAX_TRIES) {
      dbclient.setConnectionParamsV1(INFLUXDB_URL, INFLUXDB_DB_NAME, INFLUXDB_USER, INFLUXDB_PASSWORD);
      influxConnected = dbclient.validateConnection();
      if (influxConnected) {
        Serial.print("Connected to InfluxDB: "); Serial.println(dbclient.getServerUrl());
        printLine("InfluxDB connection OK", 4);
        connect_counter = INFLUX_MAX_TRIES;
      } else {
        Serial.print("InfluxDB connection failed: "); Serial.println(dbclient.getLastErrorMessage());
        printLine("*** FAILED ***", 4);
        printLine("InfluxDB connection", 5);
        printLine("problem", 6);
        connect_counter++;
      }
    }
  } else {
    printLine(F("*** WIFI FAILED ***"), 3);
  }
  if (!wifiConnected || !influxConnected) {
    Serial.println("Setup FAILED: Aborting.");
    while (1);  // loop forever
  }

  // Create tags:
  sensor.addTag("device_uid", DEVICE_UID);
  sensor.addTag("location", DEVICE_LOCATION);

  display.clearDisplay();
  printIP();

  Serial.println("Looping...");
}

// ***************************************************************************************
// ***  MAIN LOOP                                                                      ***
// ***************************************************************************************
uint8_t report_counter = 0;

void loop() {
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Wifi connection lost");
    wifiConnected = wifiConnect();
    if (wifiConnected) printIP();
  }

  temperature = sht.getTemperature();
  printTemperature(temperature);
  humidity = sht.getHumidity();
  printHumidity(humidity);
  display.display();

  if (report_counter == REPORT_INTERVAL) {
    // Send data to InfluxDB
    if (wifiConnected) {
      sensor.clearFields();
      sensor.addField("temp", temperature);
      bool result = dbclient.writePoint(sensor);
      if (result) {
        Serial.println(F("Data sent OK."));
      } else {
        Serial.println(F("Failed to send data."));
      }
    } else {
      Serial.println(F("Could not send report because wifi is down."));
    }
    report_counter = 0;
  } else {
    report_counter++;
  }

  delay(LOOP_DELAY);

}
