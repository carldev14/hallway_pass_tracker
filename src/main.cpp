#include "core/display.h"
#include "core/inputs.h"
#include "system/config.h"
#include <Arduino.h>
#include <UniversalTelegramBot.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// --- TELEGRAM CREDENTIALS ---
// Replace with your REAL token
#define BOT_TOKEN "8882655287:AAHzLTLjejSFGE7IBSxwBf1uzPooyHI12w4"
#define CHAT_ID "6829531974"

// Initialize Telegram Bot
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

void sendTelegramAlert(String studentName, String destination, int minutes) {
  // Bypass SSL certificate check (useful for school networks)
  secured_client.setInsecure();

  String message = "🚨 *HALLWAY PASS VIOLATION* 🚨\n\n";
  message += "Class: *Altair 12 - ICT*\n";
  message += "Student: *" + studentName + "*\n";
  message += "Destination: *" + destination + "*\n";
  message += "Time limit: " + String(minutes) + " minutes\n\n";
  message += "The student failed to return within given time. Take action by "
             "any means nessesary.";

  Serial.print("Sending Telegram Alert...");
  if (bot.sendMessage(CHAT_ID, message, "Markdown")) {
    Serial.println(" ✅ Sent successfully!");
  } else {
    Serial.println(" ❌ Failed to send.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 1. Initialize Web Server (Connects to router)
  DisplayOutputs::get().initialize();

  // 2. Initialize RFID
  Inputs::get().initializeInputs();

  // 3. Sync time (required for secure SSL connections)
  Serial.print("Syncing time via NTP");
  configTime(0, 0, "pool.ntp.org");
  time_t now = time(nullptr);
  while (now < 24 * 3600) {
    delay(100);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println(" ✅ Time synced.");

  SystemConfig::get().currentState = DisplayState::HOME;
}

void loop() {
  DisplayOutputs::get().handleClient();
  SystemConfig &config = SystemConfig::get();

  // STATE 1: RULES (Waiting for RFID tap)
  if (config.currentState == DisplayState::RULES) {
    if (Inputs::get().scanCard()) {
      Inputs::get().haltCard();
      config.startTimer();
      config.currentState = DisplayState::COUNTDOWN;
      Serial.println("RFID Tapped! Countdown started.");
    }
  }

  // STATE 2: COUNTDOWN
  else if (config.currentState == DisplayState::COUNTDOWN) {
    int secondsLeft = config.getSecondsRemaining();

    // SUCCESS: Card tapped in time
    if (Inputs::get().scanCard()) {
      Inputs::get().haltCard();
      config.currentState = DisplayState::SUCCESS;
      Serial.println("SUCCESS: Returned in time.");
      delay(5000);
      config.currentState = DisplayState::HOME;
      return;
    }

    // ERROR: Time ran out -> SEND TELEGRAM
    if (secondsLeft <= 0) {
      config.currentState = DisplayState::ERROR;
      Serial.println("ERROR: Time limit exceeded! Sending Telegram...");

      // Pass the student name, destination, and time limit
      sendTelegramAlert(config.studentName, config.selectedDestination,
                        config.timeLimitMinutes);

      delay(5000);
      config.currentState = DisplayState::HOME;
    }
  }
}
