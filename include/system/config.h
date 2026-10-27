#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include <Arduino.h>

// ==========================================
// 1. PIN CONFIGURATION
// ==========================================
#define RFID_SS_PIN 5
#define RFID_RST_PIN 4  // Moved to 4 to avoid LCD conflict
#define LCD_SDA_PIN 21
#define LCD_SCL_PIN 22
#define SPI_SCK_PIN 18
#define SPI_MISO_PIN 19
#define SPI_MOSI_PIN 23
#define FEEDBACK_BUZZER 26
#define FEMALE_SLOT 32
#define MALE_SLOT 33


// ==========================================
// 2. DISPLAY STATES
// ==========================================
enum class DisplayState { SCAN_PASS, HOME, SCAN_ID, COUNTDOWN, SUCCESS, ERROR };

// ==========================================
// 3. SYSTEM CONFIG (Singleton)
// ==========================================
class SystemConfig {
public:
  static SystemConfig &get() {
    static SystemConfig instance;
    return instance;
  }

  // ==========================================
  // AUTHORIZED CARDS
  // ==========================================
  const byte AuthorizedMalePassCard[4]   = {0x1B, 0xD9, 0xB8, 0xE3};
  const byte AuthorizedFemalePassCard[4] = {0xB4, 0x3F, 0xB9, 0xE3};
  const byte AuthorizedStudentID_1[4] = {0x90, 0x04, 0x40, 0x64}; // Robin (Male)
  const byte AuthorizedStudentID_2[4] = {0xE0, 0xE6, 0x03, 0x64}; // Emman (Female)

  // ==========================================
  // APP STATE VARIABLES
  // ==========================================
  DisplayState currentState = DisplayState::SCAN_PASS; 
  String passCardGender = ""; 
  String studentName = "";    
  String selectedDestination = "";
  int timeLimitMinutes = 0;
  unsigned long countdownEndTime = 0;
  
  // Store the IP address to show on the LCD
  String serverIP = ""; 

  // ==========================================
  // HELPER METHODS
  // ==========================================
  void setDestination(const String &dest) {
    selectedDestination = dest;
    if (dest == "Faculty") timeLimitMinutes = 15;
    else if (dest == "Restroom") timeLimitMinutes = 10;
    else if (dest == "Clinic") timeLimitMinutes = 20;
    else timeLimitMinutes = 0;
  }

  void startTimer() {
    countdownEndTime = millis() + (timeLimitMinutes * 60 * 1000UL);
  }

  int getSecondsRemaining() {
    unsigned long now = millis();
    if (now >= countdownEndTime) return 0;
    return (countdownEndTime - now) / 1000;
  }

  String getPassGender(byte* uid) {
    if (memcmp(uid, AuthorizedMalePassCard, 4) == 0) return "Male";
    if (memcmp(uid, AuthorizedFemalePassCard, 4) == 0) return "Female";
    return "";
  }

  String getStudentName(byte* uid) {
    if (memcmp(uid, AuthorizedStudentID_1, 4) == 0) return "Robin Carl Sipocot";
    if (memcmp(uid, AuthorizedStudentID_2, 4) == 0) return "Emman Layug";
    return "";
  }

  String getStudentGender(byte* uid) {
    if (memcmp(uid, AuthorizedStudentID_1, 4) == 0) return "Male";
    if (memcmp(uid, AuthorizedStudentID_2, 4) == 0) return "Female";
    return "";
  }

private:
  SystemConfig() = default;
  ~SystemConfig() = default;
  SystemConfig(const SystemConfig &) = delete;
  SystemConfig &operator=(const SystemConfig &) = delete;
};

#endif // SYSTEM_CONFIG_H