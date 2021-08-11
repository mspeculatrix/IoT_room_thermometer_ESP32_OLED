# IoT_room_thermometer_ESP32_OLED

ESP32-based IoT room thermometer with InfluxDB client & OLED display.

Version: 1.0  ::  09/08/2021  

Machina Speculatrix :: https://mansfield-devine.com/speculatrix/
 
Open source - have at it!
  
This code comes with no guarantees or promises. Don't blame me if it burns your house down.

For a TTGO ESP32 dev board with 128x64 OLED display.
 
Uses a GY-21 (SHT21/HTU21) I2C temperature & humidity sensor module.
 
Because of the dev board's unusual I2C pinout – used for the OLED display - we need a TwoWire instance with custom pin assignments to pass to the Adafruit_SSD1306 instance.

For the same reason, the Environment library in the Arduino IDE doesn't work for use wth the GY-21 sensor board.
Neither did the SHT21 library. So I modified the latter to have a constructor that allows me to pass the same TwoWire instance that I created for the OLED display. This version is called SHT21_TTGO. It will get its own GitHub repo soon.

Display reset pin is 16.
Display I2C address is 0x3C (normally 0x3D for a 128x64 OLED).
  
The code uses lots of Serial.print() statements for debugging purposes. These can be - and probably should be - removed or commented out.

Board: ESP32 Dev Module
  
NOTE: Wifi credentials and InfluxDB settings (including credentials) are stored in header files in my Arduino libraries folder.

In my setup, I have two wifi APs, which is why you see the connection routine alternate between the two. The WifiCfgHome.h header file looks something like this:

#define WIFI_SSID_MAIN "ssid1"
#define WIFI_SSID_ALT "ssid2"
#define WIFI_PASSWORD "password"
  
You could always use the same setup and just make both SSIDs the same, if you have only one AP (poor you!).

The InfluxDbConfig.h looks a bit like this:

#define INFLUXDB_URL "http://10.0.30.50:8086"
#define INFLUXDB_DB_NAME "iot"
#define INFLUXDB_USER "iot"
#define INFLUXDB_PASSWORD "password"
