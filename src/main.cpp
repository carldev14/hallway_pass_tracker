#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include "core/display.h"
#include "core/inputs.h"
#include "system/config.h"

#define BOT_TOKEN "8882655287:AAHzLTLjejSFGE7IBSxwBf1uzPooyHI12w4"
#define CHAT_ID   "6829531974"

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

void sendTelegramAlert(String studentName, String gender, String destination, int minutes) {
  secured_client.setInsecure(); 
  String message = "🚨 *HALLWAY PASS VIOLATION* 🚨\n\n";
  message += "Class: *Altair 12 - ICT*\n";
  message += "Student: *" + studentName + "*\n";
  message += "Gender: *" + gender + "*\n";
  message += "Destination: *" + destination + "*\n";
  message += "Time Limit: " + String(minutes) + " minutes\n\n";
  message += "The student failed to return the card within the given time.";
  
  if (bot.sendMessage(CHAT_ID, message, "Markdown")) Serial.println("✅ Telegram sent.");
  else Serial.println("❌ Telegram failed.");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  DisplayOutputs::get().initialize();
  Inputs::get().initializeInputs();

  Serial.print("Syncing time...");
  configTime(0, 0, "pool.ntp.org");
  time_t now = time(nullptr);
  while (now < 24 * 3600) { delay(100); now = time(nullptr); }
  Serial.println(" ✅");

  SystemConfig::get().currentState = DisplayState::SCAN_PASS;
  DisplayOutputs::get().lcdPrint("Tap PASS card", "To proceed...");
}

void loop() {
  DisplayOutputs::get().handleClient();
  SystemConfig &config = SystemConfig::get();

  // ==========================================
  // STATE 1: SCAN PASS CARD (Authentication)
  // ==========================================
  if (config.currentState == DisplayState::SCAN_PASS) {
    if (Inputs::get().scanCard()) {
      if (Inputs::get().isPassCardValid()) {
        config.currentState = DisplayState::HOME;
        
        // LCD Guide: Card Removed, visit URL
        DisplayOutputs::get().lcdPrint("Card Removed.", "Scan QrCode");
        Serial.println("Pass Card Login. Go to Dashboard.");
      } else {
        DisplayOutputs::get().lcdPrint("Access Denied.", "Use Pass Card.");
        Serial.println("❌ Invalid Card.");
      }
      Inputs::get().haltCard();
    }
  }

  // ==========================================
  // STATE 2: SCAN STUDENT ID (Verification)
  // ==========================================
  else if (config.currentState == DisplayState::SCAN_ID) {
    if (Inputs::get().scanCard()) {
      if (Inputs::get().isStudentIDValid()) {
        config.startTimer();
        config.currentState = DisplayState::COUNTDOWN;
        
        // LCD Guide: Starting timer
        DisplayOutputs::get().lcdPrint("ID Verified!", "Starting timer.");
        delay(1500);
        DisplayOutputs::get().lcdPrint("Return Card", "within time limit.");
        
        Serial.print("ID Verified. Countdown started for: ");
        Serial.println(config.studentName);
      } else {
        DisplayOutputs::get().lcdPrint("ID Failed.", "Wrong Gender/ID.");
        Serial.println("❌ ID Verification Failed.");
      }
      Inputs::get().haltCard();
    }
  }
  
  // ==========================================
  // STATE 3: COUNTDOWN (Waiting for return)
  // ==========================================
  else if (config.currentState == DisplayState::COUNTDOWN) {
    
    // Check if they returned the Pass Card
    if (Inputs::get().scanCard()) {
      if (Inputs::get().isPassCardValid()) {
        config.currentState = DisplayState::SUCCESS;
        
        // LCD Guide: Success
        DisplayOutputs::get().lcdPrint("Pass Recorded.", "Thank you!");
        Serial.println("✅ Pass Card RETURNED. Success.");
        
        delay(3000);
        config.currentState = DisplayState::SCAN_PASS;
        config.studentName = "";
        config.passCardGender = "";
        DisplayOutputs::get().lcdPrint("Tap Pass Card", "To login.");
        return;
      }
      Inputs::get().haltCard();
    }

    // Check for Timeout
    int secondsLeft = config.getSecondsRemaining();
    if (secondsLeft <= 0) {
      config.currentState = DisplayState::ERROR;
      
      // LCD Guide: Violation
      DisplayOutputs::get().lcdPrint("VIOLATION!", "Return card now.");
      Serial.println("❌ ERROR: Time limit exceeded!");
      
      sendTelegramAlert(config.studentName, config.passCardGender, config.selectedDestination, config.timeLimitMinutes);
      
      delay(5000);
      config.currentState = DisplayState::SCAN_PASS;
      config.studentName = "";
      config.passCardGender = "";
      DisplayOutputs::get().lcdPrint("Tap Pass Card", "to login.");
    }
  }
}