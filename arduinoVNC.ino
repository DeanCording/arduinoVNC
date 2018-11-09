/*
   arduinoVNC.ino

    Created on: 20.02.2016

   required librarys:
    - SPI (arduino core)
    - WiFi (arduino core)
    - WiFiManager (https://github.com/tzapu/WiFiManager)
    - TFT_eSPI (https://github.com/Bodmer/TFT_eSPI)
*/

#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#else
#include <WiFi.h>
#endif
#include <SPI.h>
#include <EEPROM.h>

#include <TFT_eSPI.h>

#include "VNC.h"

// Pin to trigger config portal
#define CONFIG_PIN 0

// Pin to pulse to sound bell
#define BELL_PIN 2

struct {
  int checksum;
  char server[40];
  char port[6];
  char password[40];
#ifdef TOUCH_CS
  uint16_t touchCalData[5];
#endif
} vnc_config;



TFT_eSPI tft = TFT_eSPI();
arduinoVNC vnc = arduinoVNC(&tft);

#ifdef TOUCH_CS
uint16_t touchX = 0, touchY = 0;
uint8_t touchPressure;
#endif

void TFTnoWifi(void) {
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, ((tft.height() / 2) - (5 * 8)));
  tft.setTextColor(TFT_RED);
  tft.setTextSize(3);
  tft.println("NO WIFI!");
  tft.setTextSize(2);
  tft.println();
}


void TFTnoVNC(void) {
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(3);
  tft.println("Connect VNC");
  tft.println();
  tft.print(vnc_config.server);
  tft.print(":");
  tft.println(vnc_config.port);
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(3);
  tft.println("Connect to ");
  tft.println(myWiFiManager->getConfigPortalSSID());
  tft.println("to configure");
  tft.println("WiFi and VNC.");
  tft.setTextSize(2);
  tft.println();
}

void writeConfig() {
  for (int address = 0; address < sizeof vnc_config; address++) {
    vnc_config.checksum += ((char *)&vnc_config)[address];
  }

  for (int address = 0; address < sizeof vnc_config; address++) {
    EEPROM.write(address, ((char *)&vnc_config)[address]);
  }
  EEPROM.commit();
}

// WiFiManager

//callback notifying us of the need to save config
bool saveConfig = false;

void saveConfigCallback () {
  Serial.println("Saving configuration...");
  saveConfig = true;
}

bool connectWifi(bool config) {

  WiFiManager wifiManager;
  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_vnc_server("server", "vnc server", vnc_config.server, 40);
  WiFiManagerParameter custom_vnc_port("port", "vnc port", vnc_config.port, 6);
  WiFiManagerParameter custom_vnc_password("password", "vnc password", vnc_config.password, 40);

  wifiManager.setDebugOutput(false);

  // If WiFi connection continually fails, uncomment this line to reset settings.
  // wifiManager.resetSettings();

  wifiManager.addParameter(&custom_vnc_server);
  wifiManager.addParameter(&custom_vnc_port);
  wifiManager.addParameter(&custom_vnc_password);

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);
  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //sets timeout until configuration portal gets turned off in seconds
  wifiManager.setTimeout(120);

  if (config) {
    Serial.println("Starting Wifi Manager portal...");
    bool result = wifiManager.startConfigPortal();

    if (saveConfig) {
      //read updated parameters
      strcpy(vnc_config.server, custom_vnc_server.getValue());
      strcpy(vnc_config.port, custom_vnc_port.getValue());
      strcpy(vnc_config.password, custom_vnc_password.getValue());
    }
    return result;
  } else {
    return wifiManager.autoConnect();
  }
}

#ifdef TOUCH_CS
// Code to run a touch screen calibration
void touch_calibrate()
{
  // Calibrate
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(20, 0);
  tft.setTextFont(2);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.println("Touch corners as indicated");

  tft.setTextFont(1);
  tft.println();

  tft.calibrateTouch(vnc_config.touchCalData, TFT_MAGENTA, TFT_BLACK, 15);

  DEBUG_VNC_TOUCH("Touch calibration data: %d, %d, %d, %d, %d\n", vnc_config.touchCalData[0], vnc_config.touchCalData[1],
                  vnc_config.touchCalData[2], vnc_config.touchCalData[3], vnc_config.touchCalData[4]);

  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.println("Calibration complete!");

  delay(4000);
}
#endif

void setup(void) {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  pinMode(CONFIG_PIN, INPUT_PULLUP);

#ifdef BELL_PIN
  pinMode(BELL_PIN, OUTPUT);
#endif

  // Init TFT
  tft.init();
  delay(10);
  tft.setRotation(1);
  tft.setTextFont(2);
  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(4);
  tft.setCursor(0, 5);
  tft.println("Arduino VNC");

  int eeprom_checksum = 0;

  EEPROM.begin(sizeof vnc_config);

  // Get VNC config info
  int address;
  for (address = 0; address < sizeof vnc_config; address++) {
    ((char *)&vnc_config)[address] = EEPROM.read(address);

    if (address > (sizeof vnc_config.checksum) - 1)  eeprom_checksum += ((char *)&vnc_config)[address];
  }

  if (vnc_config.checksum != eeprom_checksum) {
    vnc_config.checksum = 0;
    vnc_config.server[0] = 0;
    vnc_config.port[0] = 0;
    vnc_config.password[0] = 0;
#ifdef TOUCH
    vnc_config.touchCalData[0] = 0;
    vnc_config.touchCalData[1] = 0;
    vnc_config.touchCalData[2] = 0;
    vnc_config.touchCalData[3] = 0;
    vnc_config.touchCalData[4] = 0;
#endif
  }

  // We start by connecting to a WiFi network

#ifdef ESP8266
  // disable sleep mode for better data rate
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
#endif

  Serial.println();
  Serial.println();
  Serial.print("Connecting to WiFi");

  WiFiManager wifiManager;

  connectWifi(vnc_config.checksum == 0);

#ifdef TOUCH_CS
  if (vnc_config.checksum != 0) {
    tft.setTouch(vnc_config.touchCalData);
    delay(500);
    if (tft.getTouch(&touchX, &touchY)) { // Touch screen on startup to trigger recalibration
    touch_calibrate();
      vnc_config.checksum = 0;
    }
  } else {
    touch_calibrate();
  }
#endif

  if (vnc_config.checksum == 0) {
    writeConfig();
  }

  TFTnoVNC();

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println(F("[SETUP] VNC..."));

  vnc.begin(vnc_config.server, atoi(vnc_config.port));
  if (vnc_config.password[0] != 0)
    vnc.setPassword(vnc_config.password); // optional
}

void loop() {
  if (digitalRead(CONFIG_PIN) == LOW) {
    Serial.println("Config portal triggered");
    vnc.disconnect();
    WiFi.disconnect();
    connectWifi(true);
    writeConfig();

  }


  if (WiFi.status() != WL_CONNECTED) {
    vnc.reconnect();
    TFTnoWifi();
    delay(1000);
  } else {
#ifdef TOUCH_CS
    if (vnc.connected()) {
      touchPressure = tft.getTouch(&touchX, &touchY);

      if (touchPressure) {
        static unsigned long lastUpdateP;
        static unsigned long lastUpdateR;
        static uint16_t lx, ly;
        if (touchPressure > 600) {
          if ((millis() - lastUpdateP) > 20) {
            vnc.mouseEvent(touchX, touchY, 0b001);
            lx = touchX;
            ly = touchY;
            lastUpdateP = millis();
            DEBUG_VNC_TOUCH("[Touch] press: 1 X: %d Y: %d\n", touchX, touchY);
          }
          lastUpdateR = 0;
        } else {
          if ((millis() - lastUpdateR) > 20) {
            vnc.mouseEvent(lx, ly, 0b000);
            lastUpdateR = millis();
            DEBUG_VNC_TOUCH("[Touch] press: 0 X: %d Y: %d\n", lx, ly);
          }
          lastUpdateP = 0;
        }
      }
    }
#endif
  }
  
  vnc.loop();
  if (!vnc.connected()) {
    TFTnoVNC();
    // some delay to not flood the server
    delay(5000);
  }
}
