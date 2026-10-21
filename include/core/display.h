#ifndef DISPLAY_H
#define DISPLAY_H

#include "system/config.h"
#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

class DisplayOutputs {
public:
  static DisplayOutputs &get();
  void initialize();
  void handleClient();
  
  // LCD Helper
  void lcdPrint(String line1, String line2);

private:
  DisplayOutputs() : lcd(0x27, 16, 2) {} // Change 0x27 to 0x3F if screen is blank
  ~DisplayOutputs() = default;
  DisplayOutputs(const DisplayOutputs &) = delete;
  DisplayOutputs &operator=(const DisplayOutputs &) = delete;

  WebServer server{80};
  LiquidCrystal_I2C lcd;

  void setupRoutes();
  void handleRoot();
  void handleStart();
  void handleStatus();
};

#endif // DISPLAY_H