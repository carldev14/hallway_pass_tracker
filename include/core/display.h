#ifndef DISPLAY_H
#define DISPLAY_H

#include "system/config.h"
#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

class DisplayOutputs {
public:
  // ==========================================
  // SINGLETON
  // ==========================================
  static DisplayOutputs &get();

  /**
   * @brief Initializes the WiFi Access Point and starts the Web Server.
   */
  void initialize();

  /**
   * @brief Must be called continuously in the main loop to handle web requests.
   */
  void handleClient();

  /**
   * @brief Can be use to run the testing/debugging for instant demo. Instant
   * rfid bypassed, executing the countdown immediately.
   * *
   * */
  void handleSimulateRfid();

private:
  DisplayOutputs() = default;
  ~DisplayOutputs() = default;
  DisplayOutputs(const DisplayOutputs &) = delete;
  DisplayOutputs &operator=(const DisplayOutputs &) = delete;

  WebServer server{80};

  // Route handlers
  void setupRoutes();
  void handleRoot();
  void handleStart();
  void handleStatus();
};

#endif // DISPLAY_H
