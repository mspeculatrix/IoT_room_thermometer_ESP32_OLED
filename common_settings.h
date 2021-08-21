/*  common_settings.h
	Setting shared by all versions of the code. */

#define INFLUX_MEASUREMENT  "temperature"
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
#define ALERT_URGENCY       3       // on a scale of 0-9, with 9 being the highest

#define SCREEN_WIDTH  128         // OLED display width, in pixels
#define SCREEN_HEIGHT  64         // OLED display height, in pixels
#define OLED_RESET     16         // Reset pin - changed to 16 for TTGO board
#define TEMPC_X_POS 1
#define TEMPC_Y_POS 36
