/* This is the base code for a CYD
Base code includes WiFi management as well as display, LED, Serial port and non-volatile storage

eSPI settings needed for 2.8" 240x320:
#define ILI9341_DRIVER // Generic driver for common displays
#define TFT_WIDTH 240 // ST7789 240 x 240 and 240 x 320
#define TFT_HEIGHT 320 // ST7789 240 x 320
#define TFT_MISO 12  //19
#define TFT_MOSI 13  //23
#define TFT_SCLK 14  //18
#define TFT_CS 15  // Chip select control pin
#define TFT_DC 2   // Data Command control pin
#define TFT_RST -1 // Set TFT_RST to -1 if display RESET is connected to ESP32 board RST
#define TOUCH_CS 33 // 21 Chip select pin (T_CS) of touch screen

*/

// Include libraries here and remember to add them to the projects platformio.ini if needed
#include <Arduino.h>
#include <spi.h>                 // required for 2.8" display touch screen
#include <XPT2046_Touchscreen.h> // required for 2.8" CYD for touch screen functions
#include <TFT_eSPI.h>            // Open pio/libdeps.tft User_Setup configure for specific CYD
#include <WiFiManager.h>
#include <Preferences.h> // For non-volatile memory
#include <ESPmDNS.h>     // Header for mDNS functions

String Ver = "012a"; // Status text
// Custom name for your web page (e.g., http://myboard.local)
const char *mdnsName = "AmpServer";

// The 2.8 CYD touch uses some non default SPI Pins
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

// Create an instance of the Preferences & WiFi library
Preferences preferences;
// and and some shortcut names
TFT_eSPI tft = TFT_eSPI();
SPIClass mySpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
WiFiManager wm;

// Board-specific PINs
#define CYD_LED_RED 4    //  4 or 22
#define CYD_LED_GREEN 16 // 16
#define CYD_LED_BLUE 17  // 17

// Define pins based on which port you wired your TTL<>232 converter to.
#define RXD2 22 // I2C SDA pin 25on CYD board is GPIO 25, which is also RXD2 for RS232 serial to RX
#define TXD2 27 // I2C SCL pin 32 on CYD board is GPIO 32, which is also TXD2 for RS232 serial to TX
HardwareSerial RS232Serial(2);

// PWM related parameter settings for dimming screen backlight.
#define BACKLIGHT_PIN 21
#define TFT_BACKLIGHT_ON HIGH
int freq = 2000;
int channel = 0;
int resolution = 8;
int BL_Level = 100; // Backlight level 0-255

// put function declarations here:
void CYD_LED(String Color); // Valid calls: Off, Red, Green, Blue, Yellow, Cyan, Magenta, White
void configModeCallback(WiFiManager *myWiFiManager);
void drawHeader();
void drawRSSI();
void checkAmpComms();
void recvWithEndMarker();
void pollAmp();
void drawAmpValues();
void drawAmpStatus();
void serveWebPage();

// Declare tft buttons for 3 menus
TFT_eSPI_Button butt1;   // Create object "button11" of class TFT_eSPI_Button
TFT_eSPI_Button butt2;   // Create object "button12" of class TFT_eSPI_Button
TFT_eSPI_Button butt3;   // Create object "button11" of class TFT_eSPI_Button
TFT_eSPI_Button butt4;   // Create object "button12" of class TFT_eSPI_Button
TFT_eSPI_Button butt5;   // Create object "button11" of class TFT_eSPI_Button
TFT_eSPI_Button butt6;   // Create object "button12" of class TFT_eSPI_Button
TFT_eSPI_Button butt7;   // Create object "button11" of class TFT_eSPI_Button
TFT_eSPI_Button butt8;   // Create object "button12" of class TFT_eSPI_Button
TFT_eSPI_Button butt9;   // Create object "button11" of class TFT_eSPI_Button
TFT_eSPI_Button butt10;  // Create object "button12" of class TFT_eSPI_Button
TFT_eSPI_Button butt11;  // Create object "button11" of class TFT_eSPI_Button
TFT_eSPI_Button butt12;  // Create object "button12" of class TFT_eSPI_Button
TFT_eSPI_Button butt13;  // Create object "button11" of class TFT_eSPI_Button
TFT_eSPI_Button butt14;  // Create object "button12" of class TFT_eSPI_Button
TFT_eSPI_Button butt15;  // Create object "button11" of class TFT_eSPI_Button
TFT_eSPI_Button butt16;  // Create object "button12" of class TFT_eSPI_Button
TFT_eSPI_Button butt17;  // Create object "button11" of class TFT_eSPI_Button
TFT_eSPI_Button butt18;  // Create object "button12" of class TFT_eSPI_Button
TFT_eSPI_Button butt19;  // Create object "button11" of class TFT_eSPI_Button
TFT_eSPI_Button butt110; // Create object "button12" of class TFT_eSPI_Button
TFT_eSPI_Button butt21;  // Create object "button11" of class TFT_eSPI_Button
TFT_eSPI_Button butt22;  // Create object "button12" of class TFT_eSPI_Button
TFT_eSPI_Button butt23;  // Create object "button11" of class TFT_eSPI_Button
TFT_eSPI_Button butt24;  // Create object "button12" of class TFT_eSPI_Button
TFT_eSPI_Button butt25;  // Create object "button11" of class TFT_eSPI_Button
TFT_eSPI_Button butt26;  // Create object "button12" of class TFT_eSPI_Button
TFT_eSPI_Button butt27;  // Create object "button11" of class TFT_eSPI_Button
TFT_eSPI_Button butt28;  // Create object "button12" of class TFT_eSPI_Button
TFT_eSPI_Button butt29;  // Create object "button11" of class TFT_eSPI_Button
TFT_eSPI_Button butt210; // Create object "button12" of class TFT_eSPI_Button

// Variables needed for KPA500 amplifier code-----------------------------------------------------------------------------------------------------
boolean commStatus = false;
const byte numChars = 32;
char receivedChars[numChars]; // an array to store the received data

boolean newData = false;
String inString = "";
String BootMode = "null";
String AmpConnect = "null";
String OnMode = "null";
String OpMode = "null";
String BandCode = "null";
String FaultCode = "null";
String PaTemp = "null";
int TempC = 0;
int TempF = 0;
String FanMin = "fan min null";
float PaVolts = 0;
float VMin = 0;
float VMax = 0;
float PaAmps = 0;
int PoWatts = 0;
float PoSwr = 0;
int PeakWatts = 0;
float PeakSwr = 0;
int InputWatts = 0;
float PeakInputWatts = 0;
int PeakTemp = 0;
int PeakTempC = 0;
float RevNumber = 0.0;
int SerialNumber = 0;

String Tx = " ";

int KeyCheckCount = 0;

boolean AutoFanCommand = false; // trigger to run fan in low speed
boolean AutoFanButton = false;  // Button to enable auto fan temp control

int PeakPoLED = 0;
int PeakSwrLED = 0;

String TempStartPt = "85";
String TempStopPt = "80";

boolean StartFlag = false; // flags to prevent repeated commands if not needed...
boolean StopFlag = false;  //

// WebServer server(80);
//  String header;
//  Set web server port number to 80
WiFiServer HttpServer(80);
// Variable to store the HTTP request
String header;

// 4 text strings to read and write
String strL1 = "Label1";
String strD1 = "DATA1";
String strL2 = "Label2";
String strD2 = "Data2";

/*
// HTML & JavaScript webpage
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 4 Strings Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; text-align: center; margin: 0px; padding-top: 30px; }
    input { font-size: 1.2rem; padding: 5px; margin: 5px; }
    submit { padding: 10px 20px; font-size: 1.2rem; }
  </style>
</head>
<body>

    <h1>Sensor Reading: <span id="sensor-value">Loading...</span></h1>

    <script>
        setInterval(function() {
            fetch('/data') // Requests just the value from Arduino
                .then(response => response.text())
                .then(data => {
                    document.getElementById('sensor-value').innerText = data;
                })
                .catch(err => console.error('Error fetching data:', err));
        }, 500); // 500ms refresh rate
    </script>

  <h2>AmpServer Configuration</h2>
  <form action="/get" method="GET">
    <label>Button 1 Label (8 Characters) and Macro Code:</label>
    <input type="text" name="input1" maxlength="8" value="%STRL1%">
    <input type="text" name="input2" value="%STRD1%"><br>

    <label>String 3:</label>
    <input type="text" name="input3" value="%STRL2%">
    <label>String 4:</label><br>
    <input type="text" name="input4" value="%STRD2%"><br><br>
    <input type="submit" value="Update Button Configuration">
  </form>
</body>
</html>
)rawliteral";
*/

// Processor to replace placeholders with actual string variables
String processor(const String &var)
{
  if (var == "STRL1")
    return strL1;
  if (var == "STRD1")
    return strD1;
  if (var == "STRL2")
    return strL2;
  if (var == "STRD2")
    return strD2;
  return String();
}

// Current time used for timers instead of delay
unsigned long NewMs = 0;
unsigned long FastOldMs = 0;
unsigned long MedOldMs = 0;
unsigned long SlowOldMs = 0;
const long FastDelayMs = 50;
const long MedDelayMs = 250;
const long SlowDelayMs = 2500;

unsigned long ButtMs = 0;
unsigned long PressDelayMs = 3000;
bool isBeingHeld = false;

unsigned long currentMs = 0;
unsigned long previousMs = 0;
unsigned long intervalMs = 60000;

float tpx, tpy;

int ConnectTimeout = 15;
int PortalTimeout = 300;
int PortalRetry = 3;

int MenuNum = -1;   //
int NewMenuNum = 1; // Default menu here, 0 = system menu with pre-defined buttons
int MaxMenu = 1;    // 1-2 custom Menus supported with up to 10 buttons each

bool GoMetric = false; // For display on TFT

void setup() // ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss
{
  Serial.begin(115200);
  Serial.println("Beginning Setup");
  // On-board tri-color LED setup
  pinMode(CYD_LED_RED, OUTPUT);
  pinMode(CYD_LED_GREEN, OUTPUT);
  pinMode(CYD_LED_BLUE, OUTPUT);
  CYD_LED("White");
  Serial.println("Starting Setup and TFT initialization");
  tft.init();
  tft.setRotation(0); // 0 = portrait, 1 = landscape, 2 = portrait inverted, 3 = landscape inverted
  ledcSetup(channel, freq, resolution);
  ledcAttachPin(BACKLIGHT_PIN, channel);

  tft.setRotation(0);                                  // 0 = portrait, 1 = landscape, 2 = portrait inverted, 3 = landscape inverted
  preferences.begin("AppSettings", false);             // This is the namespace for local storage
  BL_Level = preferences.getInt("BackLight_Lvl", 100); // Look for backlight level, default to 100
  GoMetric = preferences.getBool("MetricFlag", false); // get metric flag for temp display
  preferences.end();
  ledcWrite(channel, BL_Level);
  // Serial.println("Backlight = " + String(BL_Level));
  // Serial.println("GoMetric = " + String(GoMetric));

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); // Adding a background colour erases previous text automatically
  tft.setTextFont(2);

  // Start the SPI for the touch screen and init the TS library
  mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(mySpi);
  ts.setRotation(1);
  tft.println(String(mdnsName) + " version " + Ver);
  tft.println("Starting CYD Setup");
  RS232Serial.end();
  delay(500);
  // Start RS232 serial port (38400 baud, standard 8N1 configuration)
  RS232Serial.begin(38400, SERIAL_8N1, RXD2, TXD2);
  tft.println("Serial Port Initialized.");

  // Initialize the button parameters 70 pixels apart
  // Parameters: &tft, centerX, centerY, width, height, outlineColor, fillColor, textColor, label, textSize
  int AddV = 7; // Adds a little vertical offset to the buttons to make more room for menus

  butt1.initButton(&tft, (tft.width() / 4), (tft.height() / 13) * 4 + AddV, 110, 36, TFT_WHITE, TFT_YELLOW, TFT_MAROON, "CLR WIFI", 1);
  butt2.initButton(&tft, ((tft.width() / 4) * 3), (tft.height() / 13) * 4 + AddV, 100, 36, TFT_WHITE, TFT_RED, TFT_YELLOW, "REBOOT", 1);
  butt3.initButton(&tft, (tft.width() / 4), (tft.height() / 13) * 6 + AddV, 110, 36, TFT_WHITE, TFT_YELLOW, TFT_BLACK, "3", 1);
  butt4.initButton(&tft, ((tft.width() / 4) * 3), (tft.height() / 13) * 6 + AddV, 100, 36, TFT_WHITE, TFT_RED, TFT_BLACK, "4", 1);
  butt5.initButton(&tft, (tft.width() / 4), (tft.height() / 13) * 8 + AddV, 110, 36, TFT_WHITE, TFT_DARKCYAN, TFT_WHITE, "DIS WIFI", 1);
  butt6.initButton(&tft, ((tft.width() / 4) * 3), (tft.height() / 13) * 8 + AddV, 100, 36, TFT_WHITE, TFT_CYAN, TFT_BLACK, "ENA WIFI", 1);
  butt7.initButton(&tft, (tft.width() / 4), (tft.height() / 13) * 10 + AddV, 110, 36, TFT_WHITE, TFT_NAVY, TFT_WHITE, "METRIC", 1);
  butt8.initButton(&tft, ((tft.width() / 4) * 3), (tft.height() / 13) * 10 + AddV, 100, 36, TFT_WHITE, TFT_BLUE, TFT_WHITE, "IMPERIAL", 1);
  butt9.initButton(&tft, (tft.width() / 4), (tft.height() / 13) * 12 + AddV, 110, 36, TFT_WHITE, TFT_DARKGREY, TFT_WHITE, "DIMMER", 1);
  butt10.initButton(&tft, ((tft.width() / 4) * 3), (tft.height() / 13) * 12 + AddV, 100, 36, TFT_WHITE, TFT_LIGHTGREY, TFT_BLACK, "BRIGHTER", 1);

  butt11.initButton(&tft, (tft.width() / 4), (tft.height() / 13) * 4 + AddV, 110, 36, TFT_WHITE, TFT_DARKGREY, TFT_BLACK, "11", 1);
  butt12.initButton(&tft, ((tft.width() / 4) * 3), (tft.height() / 13) * 4 + AddV, 100, 36, TFT_WHITE, TFT_LIGHTGREY, TFT_BLACK, "12", 1);
  butt13.initButton(&tft, (tft.width() / 4), (tft.height() / 13) * 6 + AddV, 110, 36, TFT_WHITE, TFT_DARKGREY, TFT_BLACK, "13", 1);
  butt14.initButton(&tft, ((tft.width() / 4) * 3), (tft.height() / 13) * 6 + AddV, 100, 36, TFT_WHITE, TFT_LIGHTGREY, TFT_BLACK, "14", 1);
  butt15.initButton(&tft, (tft.width() / 4), (tft.height() / 13) * 8 + AddV, 110, 36, TFT_WHITE, TFT_DARKGREY, TFT_BLACK, "15", 1);
  butt16.initButton(&tft, ((tft.width() / 4) * 3), (tft.height() / 13) * 8 + AddV, 100, 36, TFT_WHITE, TFT_LIGHTGREY, TFT_BLACK, "16", 1);
  butt17.initButton(&tft, (tft.width() / 4), (tft.height() / 13) * 10 + AddV, 110, 36, TFT_WHITE, TFT_DARKGREY, TFT_BLACK, "17", 1);
  butt18.initButton(&tft, ((tft.width() / 4) * 3), (tft.height() / 13) * 10 + AddV, 100, 36, TFT_WHITE, TFT_LIGHTGREY, TFT_BLACK, "18", 1);
  butt19.initButton(&tft, (tft.width() / 4), (tft.height() / 13) * 12 + AddV, 110, 36, TFT_WHITE, TFT_DARKGREY, TFT_BLACK, "19", 1);
  butt110.initButton(&tft, ((tft.width() / 4) * 3), (tft.height() / 13) * 12 + AddV, 100, 36, TFT_WHITE, TFT_LIGHTGREY, TFT_BLACK, "110", 1);

  butt21.initButton(&tft, (tft.width() / 4), (tft.height() / 13) * 4 + AddV, 110, 36, TFT_WHITE, TFT_DARKGREY, TFT_BLACK, "21", 1);
  butt22.initButton(&tft, ((tft.width() / 4) * 3), (tft.height() / 13) * 4 + AddV, 100, 36, TFT_WHITE, TFT_LIGHTGREY, TFT_BLACK, "22", 1);
  butt23.initButton(&tft, (tft.width() / 4), (tft.height() / 13) * 6 + AddV, 110, 36, TFT_WHITE, TFT_DARKGREY, TFT_BLACK, "23", 1);
  butt24.initButton(&tft, ((tft.width() / 4) * 3), (tft.height() / 13) * 6 + AddV, 100, 36, TFT_WHITE, TFT_LIGHTGREY, TFT_BLACK, "24", 1);
  butt25.initButton(&tft, (tft.width() / 4), (tft.height() / 13) * 8 + AddV, 110, 36, TFT_WHITE, TFT_DARKGREY, TFT_BLACK, "25", 1);
  butt26.initButton(&tft, ((tft.width() / 4) * 3), (tft.height() / 13) * 8 + AddV, 100, 36, TFT_WHITE, TFT_LIGHTGREY, TFT_BLACK, "26", 1);
  butt27.initButton(&tft, (tft.width() / 4), (tft.height() / 13) * 10 + AddV, 110, 36, TFT_WHITE, TFT_DARKGREY, TFT_BLACK, "27", 1);
  butt28.initButton(&tft, ((tft.width() / 4) * 3), (tft.height() / 13) * 10 + AddV, 100, 36, TFT_WHITE, TFT_LIGHTGREY, TFT_BLACK, "28", 1);
  butt29.initButton(&tft, (tft.width() / 4), (tft.height() / 13) * 12 + AddV, 110, 36, TFT_WHITE, TFT_DARKGREY, TFT_BLACK, "29", 1);
  butt210.initButton(&tft, ((tft.width() / 4) * 3), (tft.height() / 13) * 12 + AddV, 100, 36, TFT_WHITE, TFT_LIGHTGREY, TFT_BLACK, "210", 1);

  tft.println("Starting WiFi Manager");
  wm.setConnectRetries(PortalRetry);
  wm.setAPCallback(configModeCallback);
  wm.setConnectTimeout(ConnectTimeout);
  wm.setConfigPortalTimeout(PortalTimeout);
  WiFi.begin(); // Need to start WiFi for a moment in order to read SSID from memory...
  tft.printf("Connecting to %s\n", wm.getWiFiSSID().c_str());
  WiFi.disconnect();        //
  wm.autoConnect("CYD_AP"); // password can be added as second parameter, e.g. autoConnect("AmpServer_AP","password");
  // continue after connection or timeout...
  tft.println("WiFi Manager finished");
  if (WiFi.isConnected())
  {
    CYD_LED("Cyan");
    tft.println("Connected to " + WiFi.SSID());
    tft.println("");
    // tft.println("Using IP address " + WiFi.localIP().toString());
  }
  else
  {
    CYD_LED("Red");
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("No WiFi connection");
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    delay(1000);
  }

  // Initialize mDNS
  if (!MDNS.begin(mdnsName))
  {
    tft.println("Error setting up MDNS responder!");
    while (1)
    {
      delay(1000);
    }
  }
  tft.println("mDNS responder started.");

  HttpServer.begin(); // Start the HTTP server

  // Add HTTP service to MDNS
  MDNS.addService("http", "tcp", 80);
  tft.println(String("http://") + mdnsName + ".local");
  tft.println("or IP address to view webpage");
  tft.println("");
  tft.println("App starting in 5 seconds...");
  delay(5000);

  /*
  // Route for root webpage
  server.on("/", HTTP_GET, []()
            {
    String s = index_html;
    s.replace("%STRL1%", strL1);
    s.replace("%STRD1%", strD1);
    s.replace("%STRL2%", strL2);
    s.replace("%STRD2%", strD2);
    server.send(200, "text/html", s); });

  // Route to save input values
  server.on("/get", HTTP_GET, []()
            {
    if (server.hasArg("input1")) strL1 = server.arg("input1");
    if (server.hasArg("input2")) strD1 = server.arg("input2");
    if (server.hasArg("input3")) strD2 = server.arg("input3");
    if (server.hasArg("input4")) strL2 = server.arg("input4");

    String s = index_html;
    s.replace("%STRL1%", strL1);
    s.replace("%STRD1%", strD1);
    s.replace("%STRL2%", strL2);
    s.replace("%STRD2%", strD2);
    server.send(200, "text/html", s); });

  server.begin();
  */

  SlowOldMs = millis() - SlowDelayMs;
  MedOldMs = millis() - MedDelayMs;
  FastOldMs = millis() - FastDelayMs;

  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 0, 240, 60, TFT_NAVY);
  tft.drawRect(0, 0, 240, 60, TFT_WHITE);
}

void loop() // MAIN LOOP mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
{
  // Fastest Loops here
  // server.handleClient();

  // Slowest loops here
  NewMs = millis();
  if (NewMs > (SlowOldMs + SlowDelayMs)) // Slow speed loops here
  {
    drawHeader();
    SlowOldMs = NewMs;
  }

  // Medium loops here
  NewMs = millis();                    // First link for medium loop
  if (NewMs > (MedOldMs + MedDelayMs)) // Medium Speed loops here
  {

    drawRSSI();
    pollAmp();
    drawAmpStatus();
    if (MenuNum == 1)
    {
      drawAmpValues();
    }

    serveWebPage();

    MedOldMs = NewMs;
  }

  // Faster loops here
  NewMs = millis();                      // First line of fast  loop
  if (NewMs > (FastOldMs + FastDelayMs)) // Fast Loops here
  {

    if (NewMenuNum != MenuNum && NewMenuNum == 0)
    {
      tft.fillRect(0, 61, tft.height(), tft.width(), TFT_BLACK);
      butt1.drawButton();
      butt2.drawButton();
      // butt3.drawButton();
      // butt4.drawButton();
      butt5.drawButton();
      butt6.drawButton();
      butt7.drawButton();
      butt8.drawButton();
      butt9.drawButton();
      butt10.drawButton();
      MenuNum = NewMenuNum;
    }
    if (NewMenuNum != MenuNum && NewMenuNum == 1) // 1111111111111111111111111111111111111111111111111111111111111111111111111111111
    {
      tft.fillRect(0, 61, tft.width(), tft.height(), TFT_BLACK);

      /*
      butt11.drawButton();
      butt12.drawButton();
      butt13.drawButton();
      butt14.drawButton();
      butt15.drawButton();
      butt16.drawButton();
      butt17.drawButton();
      butt18.drawButton();
      butt19.drawButton();
      butt110.drawButton();
      */
      tft.drawString("VOLTS", 20, 225, 2);
      tft.drawString("WATTS", 20, 295, 2);
      tft.drawString("SWR", 135, 295, 2);

      MenuNum = NewMenuNum;
    }
  }
  else if (NewMenuNum != MenuNum && NewMenuNum == 2) // 22222222222222222222222222222222222222222222222222222222222222222222222222222222222222222
  {
    tft.fillRect(0, 61, tft.width(), tft.height(), TFT_BLACK);
    butt21.drawButton();
    butt22.drawButton();
    butt23.drawButton();
    butt24.drawButton();
    butt25.drawButton();
    butt26.drawButton();
    butt27.drawButton();
    butt28.drawButton();
    butt29.drawButton();
    butt210.drawButton();
    MenuNum = NewMenuNum;
  }

  bool isTouched = ts.tirqTouched() && ts.touched(); // Here if the screen has been touched
  TS_Point tp;
  int tp_x = 0;
  int tp_y = 0;

  if (!isTouched)
  {
    ButtMs = 0;
  }

  if (isTouched)
  {
    tp = ts.getPoint();                                                       // returns raw pixel coordinates from touch screen 0-4000
    tpx = tft.width() - ((static_cast<float>(tp.y) / 4000.0f) * tft.width()); // adjusted for 240/320
    tpy = (static_cast<float>(tp.x) / 3800.0f) * tft.height();
    Serial.println(String(tpx) + " & " + String(tpy));
    tp_x = int(tpx);
    tp_y = int(tpy);
    delay(100); // debounce

    if (tp_x < tft.width() / 2 && tp_y < 75)
    {
      NewMenuNum = MenuNum - 1;
      if (NewMenuNum < 0)
      {
        NewMenuNum = MaxMenu;
      }
    }

    if (tp_x > tft.width() / 2 && tp_y < 75)
    {
      NewMenuNum = MenuNum + 1;
      if (NewMenuNum > MaxMenu)
      {
        NewMenuNum = 0;
      }
    }
  }

  if (isTouched && MenuNum == 0) // Menu 0 Functions...000000000000000000000000000000000000000000000000000000
  {

    if (butt1.contains(tp_x, tp_y))
    {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.fillRect(0, 61, 320, 23, TFT_BLACK);

      if (ButtMs == 0)
      {
        ButtMs = millis();
      }
      unsigned long CountDown = ((PressDelayMs + ButtMs - millis()) / 1000);
      tft.drawCentreString("Hold to Reset WiFi " + String(int(CountDown)), tft.width() / 2, 62, 2);
      if ((millis() - ButtMs) > PressDelayMs)
      {
        wm.resetSettings();
        CYD_LED("Magenta");
        tft.drawCentreString("      WiFi Settings Cleared      ", tft.width() / 2, 62, 2);
      }
    }

    if (butt2.contains(tp_x, tp_y))
    {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.fillRect(0, 61, 320, 23, TFT_BLACK);
      tft.drawCentreString("Hold to Reboot", tft.width() / 2, 62, 2);
      if (ButtMs == 0)
      {
        ButtMs = millis();
      }
      unsigned long CountDown = ((PressDelayMs + ButtMs - millis()) / 1000);
      tft.drawCentreString("Hold to Reboot " + String(int(CountDown)), tft.width() / 2, 62, 2);
      if ((millis() - ButtMs) > PressDelayMs)
      {
        CYD_LED("White");
        ESP.restart();
        CYD_LED("White");
      }
    }

    if (butt5.contains(tp_x, tp_y))
    {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.fillRect(0, 61, 320, 23, TFT_BLACK);
      if (ButtMs == 0)
      {
        ButtMs = millis();
      }
      unsigned long CountDown = ((PressDelayMs + ButtMs - millis()) / 1000);
      tft.drawCentreString("Hold to Disable WiFi " + String(int(CountDown)), tft.width() / 2, 62, 2);
      if ((millis() - ButtMs) > PressDelayMs)
      {
        WiFi.disconnect();
        CYD_LED("Yellow");
        tft.drawCentreString("    WiFi Disabled    ", tft.width() / 2, 62, 2);
      }
    }

    if (butt6.contains(tp_x, tp_y))
    {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.fillRect(0, 61, 320, 23, TFT_BLACK);
      tft.drawCentreString("WiFi Reconnecting", tft.width() / 2, 62, 2);
      WiFi.reconnect();
      CYD_LED("Cyan");
    }

    if (butt7.contains(tp_x, tp_y))
    {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.fillRect(0, 61, 320, 23, TFT_BLACK);
      tft.drawCentreString("Metric Units Enabled", tft.width() / 2, 62, 2);
      GoMetric = true;
      preferences.begin("AppSettings", false);     // This is the namespace for local storage
      preferences.putBool("MetricFlag", GoMetric); // Write lev
      Serial.println("GoMetric = " + String(GoMetric));
      preferences.end();
    }

    if (butt8.contains(tp_x, tp_y))
    {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.fillRect(0, 61, 320, 23, TFT_BLACK);
      tft.drawCentreString("Imperial Units Enabled", tft.width() / 2, 62, 2);
      GoMetric = false;
      preferences.begin("AppSettings", false);     // This is the namespace for local storage
      preferences.putBool("MetricFlag", GoMetric); // Write lev
      Serial.println("GoMetric = " + String(GoMetric));
      preferences.end();
    }

    if (butt9.contains(tp_x, tp_y))
    {
      BL_Level = BL_Level - 25;
      if (BL_Level < 25)
      {
        BL_Level = 25;
      }
      ledcWrite(channel, BL_Level);
      preferences.begin("AppSettings", false);       // This is the namespace for local storage
      preferences.putInt("BackLight_Lvl", BL_Level); // Write level
      preferences.end();
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.fillRect(0, 61, 320, 23, TFT_BLACK);
      tft.drawCentreString("Backlight (25-250) = " + String(BL_Level), tft.width() / 2, 62, 2);
      delay(100); // Debounce delay
    }

    if (butt10.contains(tp_x, tp_y))
    {
      BL_Level = BL_Level + 25;
      if (BL_Level > 250)
      {
        BL_Level = 250;
      }
      ledcWrite(channel, BL_Level);
      preferences.begin("AppSettings", false);       // This is the namespace for local storage
      preferences.putInt("BackLight_Lvl", BL_Level); // Write level
      Serial.println(" Writing Backlight level: " + BL_Level);
      preferences.end();
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.fillRect(0, 61, 320, 23, TFT_BLACK);
      tft.drawCentreString("Backlight (25-250) = " + String(BL_Level), tft.width() / 2, 62, 2);
      delay(100); // Debounce delay
    }
  }

  if (isTouched && MenuNum == 1) // Menu 0 Functions...11111111111111111111111111111111111111111111111111111111111111111111
  {
  }

  if (isTouched && MenuNum == 2) // Menu 0 Functions...2222222222222222222222222222222222222222222222222222222222222222222
  {
  }

  FastOldMs = NewMs;
}

void CYD_LED(String Color)
{
  if (Color == "Off")
  {
    digitalWrite(CYD_LED_RED, HIGH);
    digitalWrite(CYD_LED_GREEN, HIGH);
    digitalWrite(CYD_LED_BLUE, HIGH);
  }
  else if (Color == "Red")
  {
    digitalWrite(CYD_LED_RED, LOW);
    digitalWrite(CYD_LED_GREEN, HIGH);
    digitalWrite(CYD_LED_BLUE, HIGH);
  }
  else if (Color == "Green")
  {
    digitalWrite(CYD_LED_RED, HIGH);
    digitalWrite(CYD_LED_GREEN, LOW);
    digitalWrite(CYD_LED_BLUE, HIGH);
  }
  else if (Color == "Blue")
  {
    digitalWrite(CYD_LED_RED, HIGH);
    digitalWrite(CYD_LED_GREEN, HIGH);
    digitalWrite(CYD_LED_BLUE, LOW);
  }
  else if (Color == "Yellow")
  {
    digitalWrite(CYD_LED_RED, LOW);
    digitalWrite(CYD_LED_GREEN, LOW);
    digitalWrite(CYD_LED_BLUE, HIGH);
  }
  else if (Color == "Magenta")
  {
    digitalWrite(CYD_LED_RED, LOW);
    digitalWrite(CYD_LED_GREEN, HIGH);
    digitalWrite(CYD_LED_BLUE, LOW);
  }
  else if (Color == "Cyan")
  {
    digitalWrite(CYD_LED_RED, HIGH);
    digitalWrite(CYD_LED_GREEN, LOW);
    digitalWrite(CYD_LED_BLUE, LOW);
  }
  else if (Color == "White")
  {
    digitalWrite(CYD_LED_RED, LOW);
    digitalWrite(CYD_LED_GREEN, LOW);
    digitalWrite(CYD_LED_BLUE, LOW);
  }
}

void configModeCallback(WiFiManager *myWiFiManager)
{
  // Turn on Red LED to indicate configuration mode
  CYD_LED("Red");

  // Update CYD Screen
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawCentreString("WiFi Portal Active", 120, 40, 4);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("Connect your device to:", 120, 90, 2);

  // Display the Access Point SSID broadcasted by the CYD
  String apSSID = myWiFiManager->getConfigPortalSSID();
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawCentreString(apSSID, 120, 120, 4);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("Go to: 192.168.4.1", 120, 170, 2);

  tft.drawCentreString("WiFi portal automatically aborts after", 120, 190, 2);
  tft.drawCentreString(String(PortalTimeout), 120, 210, 2);
  tft.drawCentreString("Seconds", 120, 230, 2);
}

void drawHeader()
{
  tft.fillRect(2, 2, 237, 18, TFT_NAVY);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);

  // tft.drawString("v" + String(Ver), 2, 2, 2);
  tft.drawString(mdnsName, 2, 2, 2);
  tft.drawCentreString(WiFi.localIP().toString(), tft.width() / 2, 2, 2);
  tft.fillTriangle(2, 35, 12, 20, 12, 50, TFT_ORANGE);
  tft.fillTriangle(tft.width() - 2, 35, tft.width() - 12, 20, tft.width() - 12, 50, TFT_ORANGE);
}

void drawRSSI()
{
  int32_t dbm = WiFi.RSSI();
  tft.setTextColor(TFT_ORANGE, TFT_NAVY);
  if (dbm > -80)
  {
    tft.setTextColor(TFT_YELLOW, TFT_NAVY);
  }
  if (dbm > -65)
  {
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
  }
  if (dbm > -50)
  {
    tft.setTextColor(TFT_CYAN, TFT_NAVY);
  }
  if (dbm == 0)
  {
    tft.setTextColor(TFT_YELLOW, TFT_RED);
  }

  tft.drawRightString(String(dbm) + " dBm", tft.width() - 3, 2, 2);
  FastOldMs = NewMs;
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

void recvWithEndMarker() // rrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr
{                        //  receive serial port data
  static byte ndx = 0;
  char endMarker = ';';
  char rc;
  delay(55); // was 50 - 18 seems to work well
  // Serial.print(" - RS232 buffer: " + String(RS232Serial.available()));
  while (RS232Serial.available() > 0 && newData == false)
  {
    rc = RS232Serial.read();
    if (rc != endMarker)
    {
      receivedChars[ndx] = rc;
      ndx++;
      if (ndx >= numChars)
      {
        ndx = numChars - 1;
      }
      delay(0);
    }
    else
    {
      receivedChars[ndx] = '\0'; // terminate the string
      ndx = 0;
      newData = true;
    }
    inString = String(receivedChars);
  }
  //  Serial.print(" > Just read InString: " + inString);
  newData = false;
  delay(0);
}

void pollAmp()
{
  recvWithEndMarker();

  // Check the status of the amplifier by sending a command to it and reading the response
  RS232Serial.print("^ON;");
  recvWithEndMarker();
  RS232Serial.print("^ON;");
  recvWithEndMarker();
  if (inString == "^ON1")
  {
    OnMode = "Power is ON";
    AmpConnect = "Serial Comm OK"; // Here we assume the amp is ON and communicating...
    // Serial.println("Power is On. inString: " + inString);
  }
  if (inString == "^ON")
  {
    OnMode = "Power is OFF";
    AmpConnect = "Serial Comm OK"; // Here we assume the amp is ON and communicating...
    // Serial.println("Power is off. inString: " + inString);
  }

  if (inString != "^ON" && inString != "^ON1")
  {
    OnMode = "null";
    AmpConnect = "Serial Comm Down. Check Mains";
    CYD_LED("Yellow"); // Turn on yellow LED to indicate amplifier communication issue
    // Serial.println(OpMode + " " + OnMode);
    BandCode = "...................................";
    OpMode = "null";
    FaultCode = "null";
    PaTemp = "null";
    TempC = 0;
    TempF = 0;
    FanMin = "null";
    PaVolts = 0;
    PaAmps = 0;
    PoWatts = 0;
    PoSwr = 0;
    InputWatts = 0;
    SerialNumber = 0;
    RevNumber = 0;
    Tx = "";
  }
  else // Contine to query data............................................
  {
    CYD_LED("Green"); // Turn on  LED to indicate amplifier communication
    // Query Firmware revision number
    if (RevNumber < 1)
    {
      RS232Serial.print("^RVM;");
      recvWithEndMarker();
      if (inString.startsWith("^RVM"))
      {
        RevNumber = (inString.substring(4, 9).toFloat());
      }
    }

    // Query Firmware serial number
    if (SerialNumber < 1)
    {
      RS232Serial.print("^SN;");
      recvWithEndMarker();
      if (inString.startsWith("^SN"))
      {
        SerialNumber = (inString.substring(3, 8).toFloat());
      }
    }

    // Query Opertional Mode of Amp
    RS232Serial.print("^OS;");
    recvWithEndMarker();
    if (inString == "^OS0")
    {
      OpMode = "STBY";
    }
    if (inString == "^OS1")
    {
      OpMode = "OPER";
    }

    // Query operating band of Amp
    RS232Serial.print("^BN;");
    recvWithEndMarker();
    BandCode = "...................................";
    if (inString == "^BN00")
    {
      BandCode = "   1.8 MHZ - 160 meters";
    }
    if (inString == "^BN01")
    {
      BandCode = "   3.5 MHZ -  80 meters  ";
    }
    if (inString == "^BN02")
    {
      BandCode = "   5.3 MHZ -  60 meters  ";
    }
    if (inString == "^BN03")
    {
      BandCode = "   7.0 MHZ -  40 meters  ";
    }
    if (inString == "^BN04")
    {
      BandCode = "  10.1 MHZ -  30 meters  ";
    }
    if (inString == "^BN05")
    {
      BandCode = "  14.0 MHZ -  20 meters  ";
    }
    if (inString == "^BN06")
    {
      BandCode = "  18.1 MHZ -  17 meters  ";
    }
    if (inString == "^BN07")
    {
      BandCode = "  21.0 MHZ -  15 meters  ";
    }
    if (inString == "^BN08")
    {
      BandCode = "  24.9 MHZ -  12 meters  ";
    }
    if (inString == "^BN09")
    {
      BandCode = "  28.0 MHZ -  10 meters  ";
    }
    if (inString == "^BN10")
    {
      BandCode = "  50.0 MHZ -  6 meters  ";
    }

    // Query Watts and SWR of PA
    RS232Serial.print("^WS;");
    recvWithEndMarker();
    PoWatts = 0;
    PoSwr = 0;
    if (inString.startsWith("^WS"))
    {
      PoWatts = (inString.substring(3, 6).toFloat());
      PoWatts = PoWatts / 1;
      PoSwr = (inString.substring(7, 10).toFloat());
      PoSwr = PoSwr / 10;
    }
    if (PoWatts > PeakWatts)
    {
      PeakWatts = PoWatts;
    }
    if (PoSwr > PeakSwr)
    {
      PeakSwr = PoSwr;
    }

    // Query Temperature of Amp
    RS232Serial.print("^TM;");
    recvWithEndMarker();
    PaTemp = "No Temp";

    if (inString.startsWith("^TM"))
    {
      PaTemp = inString.substring(3);
      TempC = (inString.substring(3).toInt());
      TempF = (TempC * 1.8) + 32;
    }

    if (TempF > PeakTemp)
    {
      PeakTemp = TempF;
    }
    if (TempC > PeakTempC)
    {
      PeakTempC = TempC;
    }

    // Query Minimum Fan Speed
    RS232Serial.print("^FC;");
    recvWithEndMarker();
    FanMin = "Off";

    if (inString.startsWith("^FC"))
    {
      FanMin = inString.substring(3);
    }

    // Query Volts and Amps of PA
    RS232Serial.print("^VI;");
    recvWithEndMarker();
    PaAmps = 0;

    if (inString.startsWith("^VI"))
    {
      PaVolts = (inString.substring(3, 6).toFloat());
      PaVolts = PaVolts / 10;

      PaAmps = (inString.substring(7, 10).toFloat());
      PaAmps = PaAmps / 10;

      InputWatts = PaVolts * PaAmps;
      if (InputWatts > PeakInputWatts)
      {
        PeakInputWatts = InputWatts;
      }
      if (((PaVolts < VMin) && (PaAmps > 1.0)) or (VMin == 0))
      {
        VMin = PaVolts;
      }
      if (PaVolts > VMax)
      {
        VMax = PaVolts;
      }
    }

    // Query Fault of Amp
    if ((PaAmps < 0.25) & (OpMode == "OPER") & (PoWatts > 20))
    { // KPA500 40 watt MAXIMUM input in OPERATE mode. This trigger requires ~30 watts in to register
      KeyCheckCount = KeyCheckCount + 1;
    }
    else
    {
      KeyCheckCount = 0;
    }

    if (KeyCheckCount > 3)
    { // Must repeat 4 times a row before fault is triggered. This count is cleared with the clear fault code button on the screen
      FaultCode = "CHECK KEYING CIRCUIT! POWER OUTPUT DETECTED WITHOUT AMP KEYED! SWITCHING TO STANDBY! ";
      RS232Serial.print("^OS0;"); // Command to Standby
      recvWithEndMarker();
      RS232Serial.print("^OS0;"); // Another command to Standby just to make sure
      recvWithEndMarker();
    }

    RS232Serial.print("^FL;");
    recvWithEndMarker();

    if ((inString == "^FL00") & (FaultCode.indexOf("STANDBY") < 1))
    { // Fault code zero if there's no keying circuit message present.
      FaultCode = "00 = No Internal Faults";
    }

    if (inString == "^FL02")
    {
      FaultCode = "02 = Excessive PA current";
    }
    if (inString == "^FL04")
    {
      FaultCode = "04 = PA temp over limit";
    }
    if (inString == "^FL06")
    {
      FaultCode = "06 = Excessive drive power";
    }
    if (inString == "^FL08")
    {
      FaultCode = "08 = 60 volt over limit";
    }
    if (inString == "^FL09")
    {
      FaultCode = "09 = Excessive reflected power";
    }
    if (inString == "^FL11")
    {
      FaultCode = "PA is dissipating excessive power";
    }
    if (inString == "^FL12")
    {
      FaultCode = "12 = Excessive power output";
    }
    if (inString == "^FL13")
    {
      FaultCode = "13 = 60 volt failure";
    }
    if (inString == "^FL14")
    {
      FaultCode = "14 = 270 volt failure";
    }
    if (inString == "^FL15")
    {
      FaultCode = "15 = Excessive overall gain";
    }
  }
}

void drawAmpStatus()
{
  tft.fillRect(16, 20, 206, 38, TFT_NAVY);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  if (OnMode == "null")
  {
    tft.setTextColor(TFT_YELLOW, TFT_MAROON);
    tft.drawCentreString(AmpConnect, 120, 22, 2);
  }
  else
  {
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    tft.drawCentreString(AmpConnect + " / " + OnMode, 120, 22, 2);
  }
  if (OpMode == "STBY")
  {
    tft.setTextColor(TFT_BLACK, TFT_YELLOW);
    tft.drawCentreString(OpMode, 120, 40, 2);
  }
  else if (OpMode == "OPER")
  {
    tft.setTextColor(TFT_BLACK, TFT_GREEN);
    tft.drawCentreString(OpMode, 120, 40, 2);
  }

  else if (FaultCode != "00 = No Internal Faults" && (OpMode != "null"))
  {
    CYD_LED("Red");
    tft.setTextColor(TFT_YELLOW, TFT_MAROON);
    tft.drawCentreString(FaultCode, 120, 40, 2);
  }
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

void drawAmpValues()
{
  if (PaAmps > 0.1)
  {
    int NewBL_Level = BL_Level * 3;
    if (NewBL_Level > 250)
    {
      NewBL_Level = 250;
    }
    ledcWrite(channel, NewBL_Level);

    tft.fillRect(0, 62, 240, 100, TFT_RED); // For ON AIR
    tft.drawRect(0, 62, 240, 100, TFT_WHITE);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.drawCentreString("ON   AIR", 120, 100, 4);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
  }
  else
  {
    // tft.fillRect(0, 108, 320, 100, TFT_BLACK); // For ON AIR
    ledcWrite(channel, BL_Level);
    // color codes from https://barth-dev.de/online/rgb565-color-picker/
    tft.fillRect(0, 62, 240, 100, 0x2800); // For ON AIR 0x2800
    tft.drawRect(0, 62, 240, 100, 0x3987);
    tft.drawRect(1, 62, 238, 98, 0x3987);
    tft.setTextColor(0x3987, 0x2800); // 2945
    tft.drawCentreString("ON   AIR", 120, 100, 4);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
  }

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(("        "), 10, 180, 6);
  if ((OnMode == "Power is OFF") or (OnMode == "null"))
  {
    tft.setTextColor(0x3987, TFT_BLACK);
  }
  if ((OnMode == "Power is ON") && ((PaVolts > 85) or (PaVolts < 60)))
  {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  }
  if ((OnMode == "Power is ON") && ((PaVolts > 90) or (PaVolts < 50)))
  {
    tft.setTextColor(TFT_RED, TFT_BLACK);
  }

  tft.drawString(String(PaVolts, 0), 10, 180, 6);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  if ((OnMode == "Power is OFF") or (OnMode == "null"))
  {
    tft.setTextColor(0x3987, TFT_BLACK);
  }

  tft.drawString(("        "), 135, 180, 6);
  if (GoMetric)
  {
    if ((OnMode == "Power is OFF") or (OnMode == "null"))
    {
      tft.setTextColor(0x3987, TFT_BLACK);
    }
    if (PaTemp.toInt() > 79) // warning temp in c
    {
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    }
    if (PaTemp.toInt() > 89) // high temp in c
    {
      tft.setTextColor(TFT_RED, TFT_BLACK);
    }

    tft.drawString((String(TempC)), 135, 180, 6);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("TEMP C", 135, 225, 2);
  }
  else
  {
    tft.drawString((String(TempF)), 135, 180, 6);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("TEMP F", 135, 225, 2);
  }

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("        ", 10, 250, 6);
  if ((OnMode == "Power is OFF") or (OnMode == "null"))
  {
    tft.setTextColor(0x3987, TFT_BLACK);
  }
  if (PaAmps > 0.1)
  {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
  }
  if ((PoWatts > 500 && (PaAmps > 0.1)))
  {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  }
  if ((PoWatts > 550) && (PaAmps > 0.1))
  {
    tft.setTextColor(TFT_RED, TFT_BLACK);
  }
  tft.drawString(String(PoWatts), 10, 250, 6);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  if (PaAmps > 0.1)
  {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
  }
  if (PoSwr > 1.5)
  {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  }
  if (PoSwr > 2.1)
  {
    tft.setTextColor(TFT_RED, TFT_BLACK);
  }

  // if (OnMode == "Power is OFF")
  if ((OnMode == "Power is OFF") or (OnMode == "null"))
  {
    tft.setTextColor(0x3987, TFT_BLACK);
  }
  tft.drawString("        ", 135, 250, 6);
  tft.drawString(String(PoSwr), 135, 250, 6);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

void serveWebPage()
{
  // Here we check the WiFi connection for activity..................
  WiFiClient client = HttpServer.available(); // Listen for incoming clients
  if (client)
  { // If a new client connects,
    // Serial.println("New Client.");  // print a message out in the serial port
    String currentLine = ""; // make a String to hold incoming data from the client
    currentMs = millis();
    previousMs = currentMs;
    while (client.connected() && currentMs - previousMs <= intervalMs)
    { // loop while the client's connected
      currentMs = millis();
      if (client.available())
      {                         // if there's bytes to read from the client,
        char c = client.read(); // read a byte, then
        // Serial.write(c);         // print it out the serial monitor
        header += c;
        if (c == '\n')
        { // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0)
          {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Issue commands to KPA500 - ------------------------------------------------------------------------------
            if (header.indexOf("GET /Power/on") >= 0)
            {
              RS232Serial.print("P");
              recvWithEndMarker();
              Serial.println("Power UP command Issued");
            }
            if (header.indexOf("GET /Power/off") >= 0)
            {
              RS232Serial.print("^ON0;");
              recvWithEndMarker();
              // Serial.println("Power DOWN command Issued");
            }

            // Issue commands to KPA500 - Operate mode
            if (header.indexOf("GET /OpMode/Operate") >= 0)
            {
              RS232Serial.print("^OS1;");
              recvWithEndMarker();
              // Serial.println("Operate command Issued");
            }

            // Issue commands to KPA500 - Standby Mode
            if (header.indexOf("GET /OpMode/Standby") >= 0)
            {
              RS232Serial.print("^OS0;");
              recvWithEndMarker();
              // Serial.println("Standby command Issued");
            }

            // Issue commands to KPA500 - Fan Min Auto
            if (header.indexOf("GET /FanMin/AutoFanButton") >= 0)
            {
              AutoFanButton = true;
            }

            // Issue commands to KPA500 - Fan Min Auto
            if (header.indexOf("GET /FanMin/FanAutoOff") >= 0)
            {
              AutoFanButton = false;
            }

            // Issue commands to KPA500 - Fan Minimum to OFF
            if (((header.indexOf("GET /FanMin/FanMin00") >= 0) || (AutoFanButton && !AutoFanCommand)) && !FanMin.startsWith("00"))
            {
              RS232Serial.print("^FC0;");
              recvWithEndMarker();
              // Serial.println("FanMin 00 Command Issued");
            }

            // Issue commands to KPA500 - Fan Minimum
            if ((header.indexOf("GET /FanMin/FanMin01") >= 0) || (AutoFanButton && AutoFanCommand && FanMin.startsWith("00")))
            { // sets minumum fan speed when in auto and temp above setpoint.
              RS232Serial.print("^FC1;");
              recvWithEndMarker();
              // Serial.println("FanMin 01 Command Issued");
            }

            // Issue commands to KPA500 - Fan Minimum
            if (header.indexOf("GET /FanMin/FanMin02") >= 0)
            {
              RS232Serial.print("^FC2;");
              recvWithEndMarker();
              // Serial.println("FanMin 02 Command Issued");
            }

            // Issue commands to KPA500 - Fan Minimum
            if (header.indexOf("GET /FanMin/FanMin03") >= 0)
            {
              RS232Serial.print("^FC3;");
              recvWithEndMarker();
              // Serial.println("FanMin 03 Command Issued");
            }

            // Issue commands to KPA500 - Fan Minimum
            if (header.indexOf("GET /FanMin/FanMin04") >= 0)
            {
              RS232Serial.print("^FC4;");
              recvWithEndMarker();
              // Serial.println("FanMin 04 Command Issued");
            }

            // Issue commands to KPA500 - Fan Minimum
            if (header.indexOf("GET /FanMin/FanMin05") >= 0)
            {
              RS232Serial.print("^FC5;");
              recvWithEndMarker();
              // Serial.println("FanMin 05 Command Issued");
            }

            // Issue commands to KPA500 - Fan Minimum
            if (header.indexOf("GET /FanMin/FanMin06") >= 0)
            {
              RS232Serial.print("^FC6;");
              recvWithEndMarker();
              // Serial.println("FanMin 06 Command Issued");
            }

            // Issue commands to KPA500 - Fault clear
            if (header.indexOf("GET /FaultCode/FaultClear") >= 0)
            {
              RS232Serial.print("^FLC;");
              recvWithEndMarker();
              KeyCheckCount = 0; // Clears auto standby reset fault
              FaultCode = "00 = Clearing Faults";
              // Serial.println("Fault CLEAR Command Issued");
            }

            // Issue commands to KPA500 - Manual Band Select
            if (header.indexOf("GET /Band/1.8") >= 0)
            {
              RS232Serial.print("^BN00;");
              recvWithEndMarker();
            }

            // Issue commands to KPA500 - Manual Band Select
            if (header.indexOf("GET /Band/3.5") >= 0)
            {
              RS232Serial.print("^BN01;");
              recvWithEndMarker();
            }

            // Issue commands to KPA500 - Manual Band Select
            if (header.indexOf("GET /Band/5.0") >= 0)
            {
              RS232Serial.print("^BN02;");
              recvWithEndMarker();
            }

            // Issue commands to KPA500 - Manual Band Select
            if (header.indexOf("GET /Band/7") >= 0)
            {
              RS232Serial.print("^BN03;");
              recvWithEndMarker();
            }
            // Issue commands to KPA500 - Manual Band Select
            if (header.indexOf("GET /Band/10") >= 0)
            {
              RS232Serial.print("^BN04;");
              recvWithEndMarker();
            }

            // Issue commands to KPA500 - Manual Band Select
            if (header.indexOf("GET /Band/14") >= 0)
            {
              RS232Serial.print("^BN05;");
              recvWithEndMarker();
            }

            // Issue commands to KPA500 - Manual Band Select
            if (header.indexOf("GET /Band/18") >= 0)
            {
              RS232Serial.print("^BN06;");
              recvWithEndMarker();
            }

            // Issue commands to KPA500 - Manual Band Select
            if (header.indexOf("GET /Band/21") >= 0)
            {
              RS232Serial.print("^BN07;");
              recvWithEndMarker();
            }

            // Issue commands to KPA500 - Manual Band Select
            if (header.indexOf("GET /Band/24") >= 0)
            {
              RS232Serial.print("^BN08;");
              recvWithEndMarker();
            }

            // Issue commands to KPA500 - Manual Band Select
            if (header.indexOf("GET /Band/28") >= 0)
            {
              RS232Serial.print("^BN09;");
              recvWithEndMarker();
            }

            // Issue commands to KPA500 - Manual Band Select
            if (header.indexOf("GET /Band/50") >= 0)
            {
              RS232Serial.print("^BN10;");
              recvWithEndMarker();
            }

            // Clear Peaks
            if (header.indexOf("GET /PeakClear/ClearPeak") >= 0)
            {
              // ---------------------------------- Peak Values are zeroed out.
              PeakWatts = 0;
              PeakSwr = 0;
              PeakInputWatts = 0;
              PeakTemp = 0;
              PeakTempC = 0;
              PeakPoLED = 0;
              PeakSwrLED = 0;
              VMax = 0;
              VMin = 0;
            }

            // -------------------------------------------------------------------------------------------------------------------  Display the HTML web page
            // -------------------------------------------------------------------------------------------------------------------  Display the HTML web page
            // -------------------------------------------------------------------------------------------------------------------  Display the HTML web page

            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\"  http-equiv=\"refresh\" content=\"2\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.print("<a href="
                         "> </a>");

            // CSS to style the on/off buttons
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html {h1 {font-size: 24px;} font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; color:Ivory; background-color:#303030;}");
            client.println(".button { background-color:whiteSmoke; color: black; padding: 8px 20px; border-color:whiteSmoke; border-radius: 8px; margin: 6px 8px; width:125px;}");
            client.println(".button2 { background-color:whiteSmoke; font-size:14px; border-radius: 8px;margin:4px 2px; width:110px;}");
            client.println(".buttonBand { background-color:whiteSmoke;font-size:14px; border-radius: 8px;margin:4px 10px; width:55px;}");
            client.println(".buttonBandOn { background-color:orange;font-size:14px; border-radius: 8px;margin:4px 10px; width:55px;}"); // #FFAA00 band button light orange
            client.println(".buttonOn { background-color:skyblue;color:black; font-size:14px;border-radius:8px;margin:4px 2px;width:110px;}");
            client.println(".buttonOper { background-color:lime;color:black; padding: 8px 20px;border-color:lime; border-radius:8px;margin:6px 8px; width:125px;}");
            client.println(".buttonStby { background-color:yellow; color: black; padding: 8px 20px;border-color:yellow;border-radius: 8px; margin: 6px 8px; width:125px;}");
            // client.println(".buttonLED {backgroun-color:grey; color:black; padding:1px  1px; border-color:black:; border-radius: 1px; margin: 1px 1px; width:25px;}");
            client.println("</style></head>");

            // Web Page Heading
            client.println("<body><h1><span style=font-family:Times New Roman,serif>");
            client.print("WD5ACP ");
            client.print(mdnsName);
            client.print(" <> ELECRAFT KPA500 </span></h1>");

            client.print("<p><a href=\"/Power/on\"><button class=\"button\">POWER ON</button></a>");
            client.println("<a href=\"/Power/off\"><button class=\"button\">POWER OFF</button></a>");
            client.println("<br>");

            if (OpMode.startsWith("OPER"))
            {
              client.print("<a href=\"/OpMode/Operate\"><button class=\"buttonOper\">OPERATE</button></a>");
            }
            else
            {
              client.print("<a href=\"/OpMode/Operate\"><button class=\"button\">OPERATE</button></a>");
            }
            if (OpMode.startsWith("ST"))
            {
              client.println("<a href=\"/OpMode/Standby\"><button class=\"buttonStby\">STANDBY</button></a>");
            }
            else
            {
              client.println("<a href=\"/OpMode/Standby\"><button class=\"button\">STANDBY</button></a>");
            }
            client.print("</p>");

            if (PaAmps < 0.25)
            { // Amp draws 0.5 amps when keyed even with no modulation.
              Tx = "&#8199";
            }
            else
            {
              Tx = "&#x2055"; // 2732
            }
            if (AmpConnect.indexOf("NOT") > 0)
            {
              client.print("<p> <span style=color:black;background-color:grey;padding:8px;border-style:inset;font-size:28px; font-family:'Courier New', monospace; >");
            }
            else
            {
              client.print("<p> <span style=color:black;background-color:#FFAA00;padding:8px;border-style:inset;font-size:28px  font-family: 'Courier New',monospace;>"); // LCD orange was #FFAA00
            }
            client.println(Tx + BandCode + "</span> </p>");

            client.print("<p>");
            client.print("<span style= font-size:10px>");
            client.print("&#9484------------------------------------------------------------- BAND -------------------------------------------------------------&#9488");
            client.print("</span><br>");

            // client.print("<p>");
            if (BandCode.indexOf("160 meters") > 0)
            {
              client.print("<a href=\"/Band/1.8\"><button class=\"buttonBandOn\">1.8</button></a>");
            }
            else
            {
              client.print("<a href=\"/Band/1.8\"><button class=\"buttonBand\">1.8</button></a>");
            }

            if (BandCode.indexOf("80") > 0)
            {
              client.print("<a href=\"/Band/3.5\"><button class=\"buttonBandOn\">3.5</button></a>");
            }
            else
            {
              client.print("<a href=\"/Band/3.5\"><button class=\"buttonBand\">3.5</button></a>");
            }

            if (BandCode.indexOf("40") > 0)
            {
              client.print("<a href=\"/Band/7.0\"><button class=\"buttonBandOn\">7</button></a>");
            }
            else
            {
              client.print("<a href=\"/Band/7.0\"><button class=\"buttonBand\">7</button></a>");
            }

            if (BandCode.indexOf("20") > 0)
            {
              client.print("<a href=\"/Band/14.\"><button class=\"buttonBandOn\">14</button></a>");
            }
            else
            {
              client.print("<a href=\"/Band/14\"><button class=\"buttonBand\">14</button></a>");
            }

            if (BandCode.indexOf("15") > 0)
            {
              client.print("<a href=\"/Band/21\"><button class=\"buttonBandOn\">21</button></a>");
            }
            else
            {
              client.print("<a href=\"/Band/21\"><button class=\"buttonBand\">21</button></a>");
            }

            if (BandCode.indexOf("10 meter") > 0)
            {
              client.print("<a href=\"/Band/28\"><button class=\"buttonBandOn\">28</button></a>");
            }
            else
            {
              client.print("<a href=\"/Band/28\"><button class=\"buttonBand\">28</button></a>");
            }

            client.print("</p>");
            client.print("<p>");

            if (BandCode.indexOf("AUX") > 0)
            {
              client.print("<a href=\"/Band/AUX\"><button class=\"buttonBandOn\">AUX</button></a>");
            }
            else
            {
              client.print("<a href=\"/Band/AUX\"><button class=\"buttonBand\">AUX</button></a>");
            }

            if (BandCode.indexOf("-  60 meters") > 0)
            {
              client.print("<a href=\"/Band/5.0\"><button class=\"buttonBandOn\">5</button></a>");
            }
            else
            {
              client.print("<a href=\"/Band/5.0\"><button class=\"buttonBand\">5</button></a>");
            }

            if (BandCode.indexOf("30") > 0)
            {
              client.print("<a href=\"/Band/10\"><button class=\"buttonBandOn\">10</button></a>");
            }
            else
            {
              client.print("<a href=\"/Band/10\"><button class=\"buttonBand\">10</button></a>");
            }

            if (BandCode.indexOf("17") > 0)
            {
              client.print("<a href=\"/Band/18\"><button class=\"buttonBandOn\">18</button></a>");
            }
            else
            {
              client.print("<a href=\"/Band/18\"><button class=\"buttonBand\">18</button></a>");
            }

            if (BandCode.indexOf("12") > 0)
            {
              client.print("<a href=\"/Band/24\"><button class=\"buttonBandOn\">24</button></a>");
            }
            else
            {
              client.print("<a href=\"/Band/24\"><button class=\"buttonBand\">24</button></a>");
            }

            if (BandCode.indexOf("6 meters") > 0)
            {
              client.print("<a href=\"/Band/50\"><button class=\"buttonBandOn\">50</button></a>");
            }
            else
            {
              client.print("<a href=\"/Band/50\"><button class=\"buttonBand\">50</button></a>");
            }
            client.print("</p>");

            client.print("<p>");
            client.print("<span style=color:ivory;font-size:12px>");
            client.println("0------------------100------------------200------------------300------------------400------------------500-------------------600------------------700");
            client.print("<br></span>");
            if (PaAmps < 0.25)
            { // Amp draws 0.5 amps when keyed even with no modulation.
              client.print("<span style=color:ivory;font-size:24px> ");
            }
            else
            {
              client.print("<span style=color:lime;font-size:24px> ");
            }
            if (PeakWatts > 525)
            {
              client.print("<span style=color:yellow;font-size:24px> ");
            }
            if (PeakWatts > 600)
            {
              client.print("<span style=color:OrangeRed;font-size:24px> ");
            }
            client.print("Watts: ");
            client.print(PoWatts);
            client.print(" W  </span>");

            int LitPoLED = (((PoWatts + 25.0) / 700.0) * 30.0) + 0.90;
            for (int PoLED = 1; PoLED < LitPoLED; PoLED++)
            {
              if (PoLED <= 25)
              {
                client.print("<span style=color:lime;font-size:24px;>");
              }
              if (PoLED > 25)
              {
                client.print("<span style=color:yellow;font-size:24px;>");
              }
              if (PoLED > 27)
              {
                client.print("<span style=color:red;font-size:24px;>");
              }
              client.print("&#9648"); // 9603
            }

            PeakPoLED = (((PeakWatts + 25.0) / 700.0) * 30.0) + 0.9;

            for (int PoLED = LitPoLED + 1; PoLED < 30; PoLED++)
            {
              if (PoLED != PeakPoLED)
              {
                client.print("<span style=color:grey;font-size:24px;>");
              }
              else
              {
                client.print("<span style=color:white;font-size:24px;>");
              }
              client.print("&#9648");
            }

            client.print("<span style=color:ivory;font-size:24px> ");
            if (PeakWatts > 525)
            {
              client.print("<span style=color:yellow;font-size:24px> ");
            }
            if (PeakWatts > 600)
            {
              client.print("<span style=color:Red;font-size:24px> ");
            }
            client.print(" Peak: ");
            client.print(PeakWatts);
            client.print(" W ");
            client.print("</span><br>");

            client.print("<span style=color:ivory;font-size:12px>");
            client.print("&nbsp");
            client.print("0--------------1--------------2--------------3--------------4--------------5");
            client.print("<br></span>");

            if (PaAmps < 0.25)
            { // Amp draws 0.5 amps when keyed even with no modulation.
              client.print("<span style=color:ivory;font-size:24px> ");
            }
            else
            {
              client.print("<span style=color:lime;font-size:24px> ");
            }
            if (PoSwr > 1.60)
            {
              client.print("<span style=color:yellow;font-size:24px> ");
            }
            if (PoSwr > 2.5)
            {
              client.print("<span style=color:Red;font-size:24px> ");
            }

            client.print("SWR: ");
            client.print(PoSwr);
            client.print("  ");

            int LitSwrLED = (((PoSwr) / 5.0) * 15.0) + 0.99;

            for (int SwrLED = 1; SwrLED < LitSwrLED; SwrLED++)
            {
              if (SwrLED <= 4)
              {
                client.print("<span style=color:lime;>");
              }
              if (SwrLED > 4)
              {
                client.print("<span style=color:yellow;>");
              }
              if (SwrLED > 7)
              {
                client.print("<span style=color:red;>");
              }
              client.print("&#9648");
            }

            PeakSwrLED = (((PeakSwr) / 5.0) * 15.0) + 0.99;

            for (int SwrLED = LitSwrLED + 1; SwrLED < 15; SwrLED++)
            {
              if (SwrLED != PeakSwrLED)
              {
                client.print("<span style=color:grey;>");
              }
              else
              {
                client.print("<span style=color:white;>");
              }
              client.print("&#9648");
            }

            client.print("<span style=color:ivory;font-size:24px> ");
            if (PeakSwr > 1.60)
            {
              client.print("<span style=color:yellow;font-size:24px> ");
            }
            if (PeakSwr > 2.5)
            {
              client.print("<span style=color:Red;font-size:24px> ");
            }
            client.print(" Peak: ");
            client.print(PeakSwr);
            client.print("  ");
            client.println("</span></p>");

            client.println("<p> <span style=font-size:24px>");
            if (TempF > 120)
            {
              client.print("<span style=color:skyblue; font-size:24px>");
            }
            if (TempF > 155)
            {
              client.print("<span style=color:orange; font-size:24px>");
            }
            // client.println(" Temp // PeakTemp: (" + PaTemp + " 'C // " + PeakTempC + " 'C) = " + TempF + " 'F // " + PeakTemp + " 'F </span> ");
            client.println("Fan Min Speed: " + FanMin + " Temp: (" + PaTemp + " 'C) " + TempF + " 'F  || Peak: (" + PeakTempC + " 'C) " + PeakTemp + " 'F </span> ");

            client.print("<br>");

            if (TempF > TempStartPt.toInt())
            { // ==============================================================    AutoFanCommand triggers min speed fan
              AutoFanCommand = true;
            }
            if (TempF < TempStopPt.toInt())
            {
              AutoFanCommand = false;
            }

            client.print("<a href=\"/FanMin/FanMin00\"><button class=\"button2\">FAN MIN 00</button></a>");

            if (AutoFanButton)
            {
              client.print("<a href=\"/FanMin/FanAutoOff\"><button class=\"buttonOn\">" + TempStopPt + "F <> " + TempStartPt + "F</button></a>");
            }

            if (!AutoFanButton)
            {
              client.print("<a href=\"/FanMin/AutoFanButton\"><button class=\"button2\">" + TempStopPt + "F <> " + TempStartPt + "F</button></a>");
            }

            if (FanMin.startsWith("01"))
            {
              client.print("<a href=\"/FanMin/FanMin01\"><button class=\"buttonOn\">FAN MIN 01</button></a>");
            }
            else
            {
              client.print("<a href=\"/FanMin/FanMin01\"><button class=\"button2\">FAN MIN 01</button></a>");
            }
            if (FanMin.startsWith("02"))
            {
              client.print("<a href=\"/FanMin/FanMin02\"><button class=\"buttonOn\">FAN MIN 02</button></a>");
            }
            else
            {
              client.print("<a href=\"/FanMin/FanMin02\"><button class=\"button2\">FAN MIN 02</button></a>");
            }
            if (FanMin.startsWith("03"))
            {
              client.print("<a href=\"/FanMin/FanMin03\"><button class=\"buttonOn\">FAN MIN 03</button></a>");
            }
            else
            {
              client.print("<a href=\"/FanMin/FanMin03\"><button class=\"button2\">FAN MIN 03</button></a>");
            }
            if (FanMin.startsWith("04"))
            {
              client.print("<a href=\"/FanMin/FanMin04\"><button class=\"buttonOn\">FAN MIN 04</button></a>");
            }
            else
            {
              client.print("<a href=\"/FanMin/FanMin04\"><button class=\"button2\">FAN MIN 04</button></a>");
            }
            if (FanMin.startsWith("05"))
            {
              client.print("<a href=\"/FanMin/FanMin05\"><button class=\"buttonOn\">FAN MIN 05</button></a>");
            }
            else
            {
              client.print("<a href=\"/FanMin/FanMin05\"><button class=\"button2\">FAN MIN 05</button></a>");
            }
            if (FanMin.startsWith("06"))
            {
              client.print("<a href=\"/FanMin/FanMin06\"><button class=\"buttonOn\">FAN MIN 06</button></a>");
            }
            else
            {
              client.print("<a href=\"/FanMin/FanMin06\"><button class=\"button2\">FAN MIN 06</button></a>");
            }
            client.print("</p>");

            if (FaultCode.startsWith("00"))
            {
              client.print("<span style=font-size:18px> ");
            }
            else
            {
              client.print("<span style=color:white;background-color:Red;font-size:18px> ");
            }
            // client.print("FaultCode = "  + FaultCode + "</span>");
            client.print("FaultCode = " + FaultCode);
            client.print("<br>");
            client.print("<a href=\"/FaultCode/FaultClear\"><button class=\"button2\">CLEAR FAULT CODES</button></a>");
            client.print("<a href=\"/PeakClear/ClearPeak\"><button class=\"button2\">CLEAR PEAK HOLDS</button></a>");
            client.print("<a href=\"/\"><button class=\"button2\">CLEAR LAST COMMAND</button></a>");
            client.print("</p>");
            client.print("<p>");

            client.print("KPA500 Power Amp Section: Volts [Min|Max]: ");
            client.print("<span style=color:lime> ");
            if ((PaVolts < 60.5) || (PaVolts > 82.9))
            {
              client.print("<span style=color:yellow> ");
            }
            if ((PaVolts < 58.5) || (PaVolts > 84.5))
            {
              client.print("<span style=color:orangered> ");
            }
            client.print(PaVolts);
            client.print("V [");
            client.print(VMin);
            client.print("V | ");
            client.print(VMax);
            client.print("V ]  ");
            client.print("<span style=color:ivory>");
            client.print("    Amps: ");
            client.print("<span style=color:lime> ");
            client.print(PaAmps);
            client.print("A   ");

            client.print("</span>");
            client.print("<span style=color:ivory> ");

            client.print("<br>");

            client.print("Peak Output Efficiency (Valid only with steady-state output): ");
            client.print(PeakWatts);
            client.print("W  ( Output ) / ");
            client.print(PeakInputWatts);
            client.print("W  ( Input ) ");
            client.print(" = ");
            client.print((PeakWatts / PeakInputWatts) * 100.0);
            client.print(" %");
            client.print("</span>");
            client.print("<br>");

            client.print("<span style=font-size:18px>");
            if (AmpConnect.indexOf("Down") > 0)
            {
              client.print("<span style=color:orangered>");
            }
            else
            {
              client.print("<span style=color:ivory>");
            }
            client.println("Status:  " + AmpConnect + "</span>");
            client.print(" || ");
            client.print(OnMode);
            client.print(" ||  KPA500 Firmware Version: ");
            client.print(RevNumber);
            client.print(" ||  KPA500 Serial Number: ");
            client.print(SerialNumber);

            client.print("<br>");

            client.print(mdnsName);
            client.printf(" Ver %s.  Refresh: 2s. WiFi: ", Ver);
            client.print(WiFi.SSID());
            client.print(". Connected ");
            client.print(WiFi.macAddress());
            client.print(" @ ");
            client.print(WiFi.localIP());
            long rssiLong = WiFi.RSSI();
            String WiFiSig = "excellent WiFi signal strength.";
            if (rssiLong < -60)
            {
              WiFiSig = "very good WiFi signal strength.";
            }
            if (rssiLong < -70)
            {
              WiFiSig = "good WiFi signal strength.";
            }
            if (rssiLong < -80)
            {
              WiFiSig = "low WiFi signal strength.";
            }
            if (rssiLong < -90)
            {
              WiFiSig = "very low WiFi signal strength.";
            }
            if (rssiLong < -99)
            {
              ;
              WiFiSig = "extremely low signal strength.";
            }
            client.print(" with ");
            client.print(rssiLong);
            client.print("dBm " + WiFiSig);

            client.print("</p>");
            client.print("</span> ");

            // The HTTP response ends with another blank line
            client.println();

            client.println("</body></html>");
            // Break out of the while loop
            break;
          }
          else
          { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        }
        else if (c != '\r')
        {                   // if you got anything else but a carriage return character,
          currentLine += c; // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    // Serial.println("Client disconnected.");
    // Serial.println("");
  }
}