/*
   ESPVNC.ino

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

struct {
  int checksum;
  char server[40];
  char port[6];
  char password[40];
} vnc_connection;

TFT_eSPI tft = TFT_eSPI();
arduinoVNC vnc = arduinoVNC(&tft);

#ifdef TOUCH
XPT2046 touch = XPT2046(TOUCH_CS, TOUCH_IRQ);
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
  tft.print(vnc_connection.server);
  tft.print(":");
  tft.println(vnc_connection.port);
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
  WiFiManagerParameter custom_vnc_server("server", "vnc server", vnc_connection.server, 40);
  WiFiManagerParameter custom_vnc_port("port", "vnc port", vnc_connection.port, 6);
  WiFiManagerParameter custom_vnc_password("password", "vnc password", vnc_connection.password, 40);

  wifiManager.setDebugOutput(true);

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

  if(config) {
    Serial.println("Starting Wifi Manager portal...");
    bool result = wifiManager.startConfigPortal();

    if (saveConfig) {
        //read updated parameters
      strcpy(vnc_connection.server, custom_vnc_server.getValue());
      strcpy(vnc_connection.port, custom_vnc_port.getValue());
      strcpy(vnc_connection.password, custom_vnc_password.getValue());
      vnc_connection.checksum = 0;
      for (int address = 0; address < sizeof vnc_connection; address++) {
        vnc_connection.checksum += ((char *)&vnc_connection)[address];
      }
  
      for (int address = 0; address < sizeof vnc_connection; address++) {
        EEPROM.write(address, ((char *)&vnc_connection)[address]);
      }
      EEPROM.commit();
    }
    return result;
  } else {
    return wifiManager.autoConnect();
  }
}


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

  // Init TFT
  tft.init();
  delay(10);
  tft.setRotation(1);
  tft.setTextFont(2);
  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(4);
  tft.setCursor(0, 5);
  tft.println("ESP VNC");

#ifdef TOUCH
  touch.begin(tft.width(), tft.height());
  touch.setRotation(1);
  touch.setCalibration(350, 550, 3550, 3600); // may need to be changed

  touch.onChange(6, 200, [](bool press, uint16_t x, uint16_t y, uint16_t z) {
    static unsigned long lastUpdateP;
    static unsigned long lastUpdateR;
    static uint16_t lx, ly;
    if (z > 600) {
      if ((millis() - lastUpdateP) > 20) {
        vnc.mouseEvent(x, y, 0b001);
        lx = x;
        ly = y;
        lastUpdateP = millis();
        Serial.printf("[Touch] press: 1 X: %d Y: %d Z: %d\n", x, y, z);
      }
      lastUpdateR = 0;
    } else {
      if ((millis() - lastUpdateR) > 20) {
        vnc.mouseEvent(lx, ly, 0b000);
        lastUpdateR = millis();
        Serial.printf("[Touch] press: 0 X: %d Y: %d Z: %d\n", lx, ly, z);
      }
      lastUpdateP = 0;
    }
  });
#endif


  int eeprom_checksum = 0;
  EEPROM.begin(sizeof vnc_connection);
  for (int address = 0; address < sizeof vnc_connection; address++) {
    ((char *)&vnc_connection)[address] = EEPROM.read(address);

    if (address > (sizeof vnc_connection.checksum) - 1)  eeprom_checksum += ((char *)&vnc_connection)[address];
  }

  if (vnc_connection.checksum != eeprom_checksum) {
    vnc_connection.server[0] = 0;
    vnc_connection.port[0] = 0;
    vnc_connection.password[0] = 0;
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

  connectWifi(vnc_connection.checksum != eeprom_checksum);

  TFTnoVNC();

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println(F("[SETUP] VNC..."));

  vnc.begin(vnc_connection.server, atoi(vnc_connection.port));
  if (vnc_connection.password[0] != 0)
    vnc.setPassword(vnc_connection.password); // optional
}

void loop() {
  if (digitalRead(CONFIG_PIN) == LOW) {
    Serial.println("Config portal triggered");
    vnc.disconnect();
    WiFi.disconnect();
    connectWifi(true);
     
  }


  if (WiFi.status() != WL_CONNECTED) {
    vnc.reconnect();
    TFTnoWifi();
    delay(500);
  } else {
#ifdef TOUCH
    if (vnc.connected()) {
      touch.loop();
    }
#endif
    vnc.loop();
    if (!vnc.connected()) {
      TFTnoVNC();
      // some delay to not flood the server
      delay(5000);
    }
  }
}
