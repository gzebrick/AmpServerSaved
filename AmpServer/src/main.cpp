/*
This program is for the CYD board. May be used with older USB-MICRO connection or newer USB-C connection.
The CYD board is a custom ESP32-based board with a (USB-C) 480x320 display and audio output. ST7796 display driver

This basic program includes:
WiFi portal for user-setting up of WiFi connections
WiFi server for display / control of data from an application
WiFi client for connecting to a server and retrieving data
Touchscreen display of data and control of the application including menus and buttons
Uses the preferences library for storing data into non-volatile memory
Support for a removable SD card for storing data and configuration files using LittleFS
Support for TTL>232 serial communication with other devices tied to the IC2 4-pin connector on the CYD board.  The I2C pins are also used for RS232 serial communication using a MAX3232 TTL<>RS232 converter.  The I2C pins are GPIO 25 (SDA) and GPIO 32 (SCL) on the CYD board.

v001 Base code
v002 Added WebServer and Host client. TFT display and menus
v003 Added funcitonal buttons and non-volatile storage

Display condfiguration for default display:
      cfg.freq_write = 55000000;
      cfg.freq_read = 20000000;
      cfg.spi_3wire = false;
      cfg.use_lock = true;
      cfg.dma_channel = 1;
      cfg.pin_sclk = 14;
      cfg.pin_mosi = 13;
      cfg.pin_miso = 12;
      cfg.pin_dc = 2;
      cfg.pin_cs = 15;
      cfg.pin_rst = -1;
      cfg.pin_busy = -1;
      touch_cs = 33;
      cfg.pin_bl = 27; // 21 for small display. HIGH to turn on backlight
      driver is ST7796, 480x320 pixels, 16-bit color, 8-bit parallel interface
*/

#define Ver "003"

#include <Arduino.h>
#include <tft_eSPI.h>    // Hardware-specific library
#include <WiFiManager.h> //
#include <Preferences.h>

// Create an instance of the Preferences library
Preferences preferences;
WiFiManager wifiManager;

#define BACKLIGHT_PIN 27
// Tri-color LED pins...
#define CYD_LED_RED 22   //  4
#define CYD_LED_GREEN 16 // 16
#define CYD_LED_BLUE 17  // 17
// #define LOW 0
// #define HIGH 1

TFT_eSPI tft = TFT_eSPI(); // pins defined in User_Setup.h  .pio/libdeps/esp32dev/TFT_eSPI/User_Setup.h

TFT_eSPI_Button butt1;  // Create object "button11" of class TFT_eSPI_Button
TFT_eSPI_Button butt2;  // Create object "button12" of class TFT_eSPI_Button
TFT_eSPI_Button butt3;  // Create object "button11" of class TFT_eSPI_Button
TFT_eSPI_Button butt4;  // Create object "button12" of class TFT_eSPI_Button
TFT_eSPI_Button butt5;  // Create object "button11" of class TFT_eSPI_Button
TFT_eSPI_Button butt6;  // Create object "button12" of class TFT_eSPI_Button
TFT_eSPI_Button butt7;  // Create object "button11" of class TFT_eSPI_Button
TFT_eSPI_Button butt8;  // Create object "button12" of class TFT_eSPI_Button
TFT_eSPI_Button butt9;  // Create object "button11" of class TFT_eSPI_Button
TFT_eSPI_Button butt10; // Create object "button12" of class TFT_eSPI_Button

bool Menu1Done = false;
bool Menu2Done = false;
bool GoMetric = false; // For temp display on TFT

// PWM related parameter settings for dimming screen backlight.  The backlight pin is defined in User_Setup.h
int freq = 2000;
int channel = 0;
int resolution = 8;
int BL_Level; // Backlight level 0-255

// Touchscreen setup stuff here
int ButtPress = 0; // Variable to store the button number pressed
int MenuNum = 1;   // Default is main menu

// put function declarations here:
void GetButtonNum(uint16_t tx, uint16_t ty);
void touch_calibrate();
int8_t getWifiQuality();
void LED_Red();
void LED_Blue();
void LED_Green();
void LED_Yellow();
void LED_White();

#include <Arduino.h>

// Define pins based on which port you wired your MAX3232 TTL<>232 converter to.
#define RXD2 25 // I2C SDA pin on CYD board is GPIO 25, which is also RXD2 for RS232 serial to RX
#define TXD2 32 // I2C SCL pin on CYD board is GPIO 32, which is also TXD2 for RS232 serial to TX
// Use HardwareSerial 2
HardwareSerial RS232Serial(2);

// Set web server port number to 80
WiFiServer HttpServer(80);
// Variable to store the HTTP request
String header;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 60000;

// Variables needed for KPA500 amplifier code-----------------------------------------------------------------------------------------------------
boolean commStatus = false;
const byte numChars = 32;
char receivedChars[numChars]; // an array to store the received data

boolean newData = false;
String inString = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
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

//=========================================================================================================================================================
// These are the routines that run based on opening the wbepage root and/or issuing a commad from the server.
//=========================================================================================================================================================

void recvWithEndMarker() // rrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr
{                        //  receive serial port data
  static byte ndx = 0;
  char endMarker = ';';
  char rc;
  delay(18); // was 50 - 18 seems to work well
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
    }
    else
    {
      receivedChars[ndx] = '\0'; // terminate the string
      ndx = 0;
      newData = true;
    }
    inString = String(receivedChars);
    delay(5); // add small delay to make sure LED is bright enough to see
  }
  // Serial.print(" > Just read InString: " + inString);
  newData = false;
}

void setup() // sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss
{
  Serial.begin(115200);
  Serial.println("AmpServer version " + String(Ver));

  preferences.begin("AppSettings", false);             // This is the namespace for local storage
  BL_Level = preferences.getInt("BackLight_Lvl", 50);  // Look for backlight level, default to 100
  GoMetric = preferences.getBool("MetricFlag", false); // get metric flag for temp display
  preferences.end();

  pinMode(CYD_LED_RED, OUTPUT);
  pinMode(CYD_LED_GREEN, OUTPUT);
  pinMode(CYD_LED_BLUE, OUTPUT);
  LED_Red(); // Turn on white LED to indicate startup

  tft.init();
  ledcSetup(channel, freq, resolution);
  ledcAttachPin(BACKLIGHT_PIN, channel);
  ledcWrite(channel, BL_Level);
  tft.setRotation(0); // 0 = portrait, 1 = landscape, 2 = portrait inverted, 3 = landscape inverted
  // Calibrate the touch screen and retrieve the scaling factors
  // touch_calibrate();
  // Replace above line with the code sent to Serial Monitor
  // once calibration is complete, e.g.:
  // uint16_t calData[5] = {286, 3534, 283, 3600, 6};
  // tft.setTouch(calData);

  // Clear the screen

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); // Adding a background colour erases previous text automatically
  tft.setTextFont(2);
  tft.println("AmpServer " + String(Ver));
  tft.print("\nWiFiManager initializing... ");
  tft.println("WiFiManager initialized");
  wifiManager.setConnectTimeout(30);
  wifiManager.setConfigPortalTimeout(300); // Set the timeout for WiFiManager Portal to 300 seconds
  tft.println("Attempting to connect to WiFi network...");
  tft.println("If no connection after 30 seconds");
  tft.println("Will start setup portal AmpServer_AP to ");
  tft.println("configure your WiFi connection");
  tft.println("at http://:192.168.4.1");
  tft.println("After configuration, the CYD board will");
  tft.println("automatically connect to the configured network");
  tft.println("Potal closes after 5 minutes");
  tft.println("Reboot or powercycle to repeat attempt");
  wifiManager.autoConnect("AmpServer_AP"); // password can be added as second parameter, e.g. autoConnect("AmpServer_AP","password");

  LED_Blue(); // Turn on blue LED to indicate WiFi connection established
  tft.println("\nnWiFi connection succesful");
  tft.print("\nWiFi connected to ");
  tft.println(WiFi.SSID());
  tft.print("IP address: ");
  tft.println(WiFi.localIP());
  HttpServer.begin(); // Start the HTTP server
  tft.println("WiFiManager finished");

  HttpServer.begin(); // Start the HTTP server
  tft.println("HTTP server started");

  // Start RS232 serial port (38400 baud, standard 8N1 configuration)
  RS232Serial.begin(38400, SERIAL_8N1, RXD2, TXD2);
  tft.println("Serial Port Initialized.");

  // Initialize the button parameters 70 pixels apart
  // Parameters: &tft, centerX, centerY, width, height, outlineColor, fillColor, textColor, label, textSize
  butt3.initButton(&tft, 80, 220, 150, 60, TFT_WHITE, TFT_MAROON, TFT_WHITE, "CLR_WIFI", 1);
  butt4.initButton(&tft, 240, 220, 150, 60, TFT_WHITE, TFT_RED, TFT_YELLOW, "REBOOT", 1);
  butt5.initButton(&tft, 80, 290, 150, 60, TFT_WHITE, TFT_DARKCYAN, TFT_WHITE, "DISC_WIFI", 1);
  butt6.initButton(&tft, 240, 290, 150, 60, TFT_WHITE, TFT_CYAN, TFT_BLACK, "ENA_WIFI", 1);
  butt7.initButton(&tft, 80, 360, 150, 60, TFT_WHITE, TFT_NAVY, TFT_WHITE, "METRIC", 1);
  butt8.initButton(&tft, 240, 360, 150, 60, TFT_WHITE, TFT_BLUE, TFT_WHITE, "IMPERIAL", 1);
  butt9.initButton(&tft, 80, 430, 150, 60, TFT_WHITE, TFT_DARKGREY, TFT_BLACK, "DIMMER", 1);
  butt10.initButton(&tft, 240, 430, 150, 60, TFT_WHITE, TFT_LIGHTGREY, TFT_BLACK, "BRIGHTER", 1);

  tft.println("Buttons initialized");
  tft.println("Application is ready");
  tft.println("Press a button to start an action");
  tft.println("or use the WiFi portal to configure the board");
  delay(2000);               // Wait for 2 seconds before drawing buttons
  tft.fillScreen(TFT_BLACK); // Clear the screen goto Menu 1
}

void loop() // llllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll
{
  // put your main code here, to run repeatedly:
  delay(500);
  // Display header information on the TFT display common for all menus
  tft.fillRect(0, 0, tft.width(), 25, TFT_NAVY); // Clear the header area
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.setCursor(tft.width() / 2 - 40, 3);
  tft.printf("AmpServer %s", Ver);
  tft.setTextColor(TFT_WHITE, TFT_BROWN);
  tft.setCursor(0, 3);
  tft.printf(" << MENU ");
  tft.setCursor(254, 3);
  tft.printf(" MENU >> ");
  tft.setTextColor(TFT_WHITE, TFT_NAVY);

  if (WiFi.status() != WL_CONNECTED) // Check if WiFi is connected
  {
    tft.fillRect(0, 25, tft.width(), 25, TFT_YELLOW); // Clear the header area
    tft.setTextColor(TFT_MAROON, TFT_YELLOW);
    int twidth = tft.textWidth("Unable to connect to " + WiFi.SSID());
    tft.setCursor((tft.width() / 2) - (twidth / 2), 28); // works off the center
    // tft.setCursor(2, 28);
    tft.print("Unable to connect to " + WiFi.SSID());
    LED_Yellow(); // Turn on yellow LED to indicate WiFi connection issue
  }
  else
  {
    tft.fillRect(0, 25, tft.width(), 25, TFT_NAVY); // Clear the header area
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    int twidth = tft.textWidth(String("WiFi  as ") + WiFi.SSID() + WiFi.localIP().toString());
    tft.setCursor((tft.width() / 2) - (twidth / 2) - 14, 28); // works off the center
    // tft.setCursor(2, 28);
    tft.printf("WiFi %s as %s", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

    tft.setCursor(275, 28);
    if (getWifiQuality() < 50)
    {
      tft.setTextColor(TFT_YELLOW, TFT_NAVY);
    }
    else
    {
      tft.setTextColor(TFT_WHITE, TFT_NAVY);
    }
    tft.printf("@ %d%%", getWifiQuality());

    LED_Green(); // Turn on green LED to indicate good WiFi connection
  }

  // Check the status of the amplifier by sending a command to it and reading the response
  RS232Serial.print("^ON;");
  recvWithEndMarker();
  delay(20);
  // RS232Serial.print("^ON;");
  // recvWithEndMarker();
  if (inString == "^ON1")
  {
    OnMode = "KPA500 Power is ON";
    Serial.println("Power is On. inString: " + inString);
  }
  else if (inString == "^ON")
  {
    OnMode = "KPA500 Power is OFF";
    Serial.println("Power is off. inString: " + inString);
  }
  else
  {
    OnMode = "null";
  }
  AmpConnect = "Serial Comm OK"; // Here we assume the amp is ON and communicating...
  if ((OnMode == "null") or (OnMode == "KPA500 Power is OFF"))
  { // Here there's no response from the amp on the serial port...

    tft.fillRect(0, 50, tft.width(), 25, TFT_MAROON); // Clear the header area
    tft.setTextColor(TFT_WHITE, TFT_MAROON);
    int twidth = tft.textWidth("Amplifier OFF Check Power.");
    tft.setCursor((tft.width() / 2) - (twidth / 2), 53); // works off the center
    tft.print("Amplifier OFF Check Power");
    LED_Yellow(); // Turn on yellow LED to indicate amplifier communication issue
    delay(100);
    AmpConnect = "Serial Communication NOT updating. Amplifier Power ON?";
    // OnMode = "null";
    OpMode = "On mode null";
    BandCode = "...................................";
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
    // tft.print(AmpConnect); // Display current AmpConnect mode on local display
  }
  else // Contine to query data............................................
  {
    // tft.print(AmpConnect); // Display current AmpConnect mode on local display
    tft.fillRect(0, 50, tft.width(), 25, TFT_NAVY); // Clear the header area
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    int twidth = tft.textWidth("Amplifier Mode" + OpMode);
    tft.setCursor((tft.width() / 2) - (twidth / 2), 53); // works off the center
    tft.printf("Amplifier Mode: %s", OpMode);
    LED_Green(); // Turn on green LED to indicate amplifier is responding

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
      if (((PaVolts < VMin) && (PaAmps > 1.0)) || (VMin == 0))
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
      FaultCode = "02 = Excessive power amplifier current";
    }
    if (inString == "^FL04")
    {
      FaultCode = "04 = Power amplifier temperature over limit";
    }
    if (inString == "^FL06")
    {
      FaultCode = "06 = Excessive driving power";
    }
    if (inString == "^FL08")
    {
      FaultCode = "08 = 60 volt supply over limit";
    }
    if (inString == "^FL09")
    {
      FaultCode = "09 = Excessive reflected power (high SWR)";
    }
    if (inString == "^FL11")
    {
      FaultCode = "Power amplifiers are dissipating excessive power";
    }
    if (inString == "^FL12")
    {
      FaultCode = "12 = Excessive power output";
    }
    if (inString == "^FL13")
    {
      FaultCode = "13 = 60 volt supply failure";
    }
    if (inString == "^FL14")
    {
      FaultCode = "14 = 270 volt supply failure";
    }
    if (inString == "^FL15")
    {
      FaultCode = "15 = Excessive overall amplifier gain";
    }
  }

  tft.fillRect(0, 75, 320, 30, TFT_NAVY); // For
  if (FaultCode != "00 = No Internal Faults")
  {
    tft.setTextColor(TFT_YELLOW, TFT_MAROON);
  }
  else
  {
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
  }
  tft.drawCentreString(FaultCode, 170, 78, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  if (MenuNum != 2) // Default menu is 1 // mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
  {
    if (Menu1Done == false)
    {
      tft.fillRect(0, 108, 320, 372, TFT_BLACK); // clear bottom of the screen
      tft.drawCircle(78, 280, 60, TFT_ORANGE);
      tft.drawString("VOLTS", 65, 305, 2);
      tft.drawCircle(242, 280, 60, TFT_ORANGE);
      // tft.drawString("TEMP F", 228, 305, 2);
      tft.drawCircle(78, 410, 60, TFT_ORANGE);
      tft.drawString("AMPS", 65, 435, 2);
      tft.drawCircle(242, 410, 60, TFT_ORANGE);
      tft.drawString("WATTS", 228, 435, 2);
      Menu1Done = true;
      Menu2Done = false;
    }

    if (PoWatts > 0.1)
    {
      int NewBL_Level = BL_Level * 3;
      if (NewBL_Level > 250)
      {
        NewBL_Level = 250;
      }
      ledcWrite(channel, NewBL_Level);

      tft.fillRect(0, 108, 320, 100, TFT_RED); // For ON AIR
      tft.drawRect(0, 108, 320, 100, TFT_WHITE);
      tft.setTextColor(TFT_WHITE, TFT_RED);
      tft.drawCentreString("ON AIR", 170, 148, 4);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
    }
    else
    {
      // tft.fillRect(0, 108, 320, 100, TFT_BLACK); // For ON AIR
      ledcWrite(channel, BL_Level);
      // color codes from https://barth-dev.de/online/rgb565-color-picker/
      tft.fillRect(0, 108, 320, 100, 0x4208); // For ON AIR
      tft.drawRect(0, 108, 320, 100, TFT_DARKGREY);
      tft.setTextColor(0x1000, 0x4208);
      tft.drawCentreString("ON AIR", 170, 148, 4);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
    }

    tft.drawString(("       "), 30, 260, 6);
    tft.drawString(String(PaVolts, 1), 30, 260, 6);

    tft.drawString(("       "), 196, 260, 6);
    if (GoMetric)
    {
      tft.drawString((String(TempC)), 196, 260, 6);
      tft.drawString("TEMP C", 228, 305, 2);
    }
    else
    {
      tft.drawString((String(TempF)), 196, 260, 6);
      tft.drawString("TEMP F", 228, 305, 2);
    }

    tft.drawString("        ", 30, 390, 6);
    tft.drawString(String(PaAmps, 1), 30, 390, 6);

    tft.drawString("        ", 196, 390, 6);
    tft.drawString(String(PoWatts), 196, 390, 6);

  } // end of Menu 1

  if (MenuNum == 2)
  { // mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
    if (Menu2Done == false)
    {
      tft.fillRect(0, 108, 320, 372, TFT_BLACK); // clear bottom of the screen
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      butt3.drawButton();
      butt4.drawButton();
      butt5.drawButton();
      butt6.drawButton();
      butt7.drawButton();
      butt8.drawButton();
      butt9.drawButton();
      butt10.drawButton();
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawCentreString(String("Mac " + WiFi.macAddress()), 170, 110, 4);
      Menu2Done = true;
      Menu1Done = false;
    }
  }

  // Check for touch input and determine which button was pressed
  uint16_t t_x = 0,
           t_y = 0; // To store the touch coordinates
  if (tft.getTouch(&t_x, &t_y))
  {
    GetButtonNum(t_x, t_y); // Call the function to get the button number based on touch coordinates
    Serial.printf("ButtPress %d Touch coordinates: x=%d, y=%d\n", ButtPress, t_x, t_y);
  }

  if (ButtPress == 3)
  {
    wifiManager.resetSettings();
  }
  else if (ButtPress == 4)
  {
    ESP.restart();
  }
  else if (ButtPress == 5)
  {
    WiFi.disconnect();
  }
  else if (ButtPress == 6)
  {
    WiFi.reconnect();
  }
  else if (ButtPress == 7)
  {
    GoMetric = true;
    preferences.begin("AppSettings", false);    // This is the namespace for local storage
    preferences.putInt("MetricFlag", GoMetric); // Write level
    preferences.end();
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawCentreString(String("   METRIC SELECTED    "), 170, 138, 4);
  }
  else if (ButtPress == 8)
  {
    GoMetric = false;
    preferences.begin("AppSettings", false);    // This is the namespace for local storage
    preferences.putInt("MetricFlag", GoMetric); // Write level
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawCentreString(String("IMPRERIAL SELECTED"), 170, 138, 4);
    preferences.end();
  }
  else if (ButtPress == 9)
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
    tft.drawCentreString("      ", 170, 162, 4);
    tft.drawCentreString(String(BL_Level), 170, 162, 4);
    delay(100);    // Debounce delay
    ButtPress = 0; // Reset button press
  }

  else if (ButtPress == 10)
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
    tft.drawCentreString("      ", 170, 162, 4);
    tft.drawCentreString(String(BL_Level), 170, 162, 4);

    delay(100);    // Debounce delay
    ButtPress = 0; // Reset button press
  }

  else if (ButtPress == 11)
  {
    MenuNum = 1;
    delay(100);    // Debounce delay
    ButtPress = 0; // Reset button press
  }
  else if (ButtPress == 12)
  {
    MenuNum = 2;
    delay(100);    // Debounce delay
    ButtPress = 0; // Reset button press
  }

  /*
    {
      if ((t_x >= tft.width() / 2 - 128) && (t_x < tft.width() / 2 + 128) && t_y > 179 && t_y < 199)
      {
        tft.setTextColor(TFT_RED, TFT_WHITE);
        tft.fillRoundRect(t_x - 10, 180, tft.width() / 2 + 138 - t_x + 10, 21, 10, 0x8430);
        tft.fillRoundRect(tft.width() / 2 - 138, 180, t_x - tft.width() / 2 + 148, 21, 10, 0x07FF);
        tft.fillCircle(t_x, 190, 10, TFT_MAGENTA);
        tft.setTextPadding(50);
        tft.drawNumber((long)t_x - (tft.width() / 2 - 128), tft.width() / 2 - 15, 115, 2);
        ledcWrite(channel, t_x - (tft.width() / 2 - 128));
      }
    }
      */

  // Here we check the WiFi connection for activity..................
  WiFiClient client = HttpServer.available(); // Listen for incoming clients
  if (client)
  { // If a new client connects,
    // Serial.println("New Client.");  // print a message out in the serial port
    String currentLine = ""; // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime)
    { // loop while the client's connected
      currentTime = millis();
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
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; color:Ivory; background-color:#303030;}");
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
            client.println("WD5ACP ELECRAFT KPA500 <> WEBSERVER </span></h1>");

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
              client.print("<span style=font-size:24px> ");
            }
            else
            {
              client.print("<span style=color:white;background-color:Red;font-size:24px> ");
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
            client.print("</p>");

            client.print("<p>");

            client.print("<span style=font-size:18px>");
            if (AmpConnect.indexOf("NOT") > 0)
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

            client.printf("WebServer Version %s.  Refresh: 2 secs. WiFi: ", Ver);
            client.print(WiFi.SSID());
            client.print(". Connected   ");
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

void GetButtonNum(uint16_t tx, uint16_t ty) // ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
{
  int xFactor = tft.width() / 6;  // Assuming 320 is the base width for scaling
  int yFactor = tft.height() / 2; // Assuming 480 is the base height for scaling
  // Serial.printf("yFactor: %d, xFactor: %d\n", yFactor, xFactor);

  // Buttons 11 and 12 are for the top menu area swipe left or right
  // Buttons 1-10 are for the lower area of the screen,
  // divided into 6 equal sections horizontally and 2 equal sections vertically

  if ((ty < yFactor) && (tx < xFactor * 1))
  {
    Serial.println("Button 11 pressed");
    ButtPress = 11;
  }
  else if ((ty > yFactor) && (tx < xFactor * 1))
  {
    Serial.println("Button 12 pressed");
    ButtPress = 12;
  }

  else if ((ty < yFactor) && (tx > xFactor * 1) && (tx < xFactor * 2))
  {
    Serial.println("Button 1 pressed");
    ButtPress = 1;
  }
  else if ((ty > yFactor) && (tx > xFactor * 1) && (tx < xFactor * 2))
  {
    Serial.println("Button 2 pressed");
    ButtPress = 2;
  }

  else if ((ty < yFactor) && (tx > xFactor * 2) && (tx < xFactor * 3))
  {
    Serial.println("Button 3 pressed");
    ButtPress = 3;
  }
  else if ((ty > yFactor) && (tx > xFactor * 2) && (tx < xFactor * 3))
  {
    Serial.println("Button 4 pressed");
    ButtPress = 4;
  }

  else if ((ty < yFactor) && (tx > xFactor * 3) && (tx < xFactor * 4))
  {
    Serial.println("Button 5 pressed");
    ButtPress = 5;
  }
  else if ((ty > yFactor) && (tx > xFactor * 3) && (tx < xFactor * 4))
  {
    Serial.println("Button 6 pressed");
    ButtPress = 6;
  }

  else if ((ty < yFactor) && (tx > xFactor * 4) && (tx < xFactor * 5))
  {
    Serial.println("Button 7 pressed");
    ButtPress = 7;
  }
  else if ((ty > yFactor) && (tx > xFactor * 4) && (tx < xFactor * 5))
  {
    Serial.println("Button 8 pressed");
    ButtPress = 8;
  }

  else if ((ty < yFactor) && (tx > xFactor * 5) && (tx < xFactor * 6))
  {
    Serial.println("Button 9 pressed");
    ButtPress = 9;
  }
  else if ((ty > yFactor) && (tx > xFactor * 5) && (tx < xFactor * 6))
  {
    Serial.println("Button 10 pressed");
    ButtPress = 10;
  }

  else
  {
    Serial.println("No button pressed");
    ButtPress = 0;
  }
}

// converts the dBm to a range between 0 and 100%
int8_t getWifiQuality()
{ // -----------------------------------------------------------------------------------------
  int32_t dbm = WiFi.RSSI();
  if (dbm <= -100)
  {
    return 0;
  }
  else if (dbm >= -50)
  {
    return 100;
  }
  else
  {
    return 2 * (dbm + 100);
  }
}

// put function definitions here:
// Code to run a screen calibration, not needed when calibration values set in setup()
void touch_calibrate() // tttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttTTT
{
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // Calibrate
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(20, 0);
  tft.setTextFont(2);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.println("Touch corners as indicated");

  tft.setTextFont(1);
  tft.println();

  tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

  Serial.println();
  Serial.println();
  Serial.println("// Use this calibration code in setup():");
  Serial.print("  uint16_t calData[5] = ");
  Serial.print("{ ");

  for (uint8_t i = 0; i < 5; i++)
  {
    Serial.print(calData[i]);
    if (i < 4)
      Serial.print(", ");
  }

  Serial.println(" };");
  Serial.print("  tft.setTouch(calData);");
  Serial.println();
  Serial.println();

  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.println("Calibration complete!");
  tft.println("Calibration code sent to Serial port.");

  delay(2000);
}

void LED_Off()
{
  digitalWrite(CYD_LED_RED, HIGH);
  digitalWrite(CYD_LED_GREEN, HIGH);
  digitalWrite(CYD_LED_BLUE, HIGH);
}
void LED_Red()
{
  digitalWrite(CYD_LED_RED, LOW);
  digitalWrite(CYD_LED_GREEN, HIGH);
  digitalWrite(CYD_LED_BLUE, HIGH);
}
void LED_Green()
{
  digitalWrite(CYD_LED_RED, HIGH);
  digitalWrite(CYD_LED_GREEN, LOW);
  digitalWrite(CYD_LED_BLUE, HIGH);
}
void LED_Blue()
{
  digitalWrite(CYD_LED_RED, HIGH);
  digitalWrite(CYD_LED_GREEN, HIGH);
  digitalWrite(CYD_LED_BLUE, LOW);
}
void LED_Yellow()
{
  digitalWrite(CYD_LED_RED, LOW);
  digitalWrite(CYD_LED_GREEN, LOW);
  digitalWrite(CYD_LED_BLUE, HIGH);
}
void LED_Magenta()
{
  digitalWrite(CYD_LED_RED, LOW);
  digitalWrite(CYD_LED_GREEN, HIGH);
  digitalWrite(CYD_LED_BLUE, LOW);
}
void LED_Cyan()
{
  digitalWrite(CYD_LED_RED, HIGH);
  digitalWrite(CYD_LED_GREEN, LOW);
  digitalWrite(CYD_LED_BLUE, LOW);
}
void LED_White()
{
  digitalWrite(CYD_LED_RED, LOW);
  digitalWrite(CYD_LED_GREEN, LOW);
  digitalWrite(CYD_LED_BLUE, LOW);
}