#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include <Arduino.h>

// ==========================================
// 1. PIN CONFIGURATION
// ==========================================
// --- RC522 RFID Module Pins ---
#define RFID_SS_PIN 5
#define RFID_RST_PIN 22

// --- Shared SPI Bus Pins (Used by the RFID reader) ---
#define SPI_SCK_PIN 18
#define SPI_MISO_PIN 19
#define SPI_MOSI_PIN 23

// ==========================================
// 2. DISPLAY STATES
// ==========================================
enum class DisplayState { HOME, RULES, COUNTDOWN, SUCCESS, ERROR };

// ==========================================
// 3. SYSTEM CONFIG (Singleton)
// ==========================================
class SystemConfig {
public:
  static SystemConfig &get() {
    static SystemConfig instance;
    return instance;
  }

  // --- Authorized RFID ID ---
  // UID: B4 3F B9 E3
  const byte authorizedHallwayPass[4] = {0xB4, 0x3F, 0xB9, 0xE3};

  // ==========================================
  // APP STATE VARIABLES
  // ==========================================
  DisplayState currentState = DisplayState::HOME;
  String studentName = "";
  String selectedDestination = "";
  int timeLimitMinutes = 0;
  unsigned long countdownEndTime = 0;

  // ==========================================
  // HELPER METHODS
  // ==========================================

  /**
   * @brief Sets the destination and calculates the time limit.
   * Faculty -> 15m, Restroom -> 10m, Clinic -> 20m
   */
  void setDestination(const String &dest) {
    selectedDestination = dest;

    if (dest == "Faculty") {
      timeLimitMinutes = 15;
    } else if (dest == "Restroom") {
      timeLimitMinutes = 10;
    } else if (dest == "Clinic") {
      timeLimitMinutes = 20;
    } else {
      timeLimitMinutes = 0;
    }
  }

  /**
   * @brief Starts the countdown timer.
   */
  void startTimer() {
    countdownEndTime = millis() + (timeLimitMinutes * 60 * 1000UL);
  }

  /**
   * @brief Gets the remaining time in seconds.
   */
  int getSecondsRemaining() {
    unsigned long now = millis();
    if (now >= countdownEndTime)
      return 0;
    return (countdownEndTime - now) / 1000;
  }

private:
  SystemConfig() = default;
  ~SystemConfig() = default;
  SystemConfig(const SystemConfig &) = delete;
  SystemConfig &operator=(const SystemConfig &) = delete;
};

#endif // SYSTEM_CONFIG_H
