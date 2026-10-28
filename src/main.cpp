#include "core/display.h"
#include "core/inputs.h"
#include "esp32-hal.h"
#include "system/config.h"
#include <Arduino.h>
#include <UniversalTelegramBot.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#define BOT_TOKEN "8882655287:AAHzLTLjejSFGE7IBSxwBf1uzPooyHI12w4"
#define CHAT_ID "-1004340850040"

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

void sendTelegramAlert(String studentName, String gender, String destination,
                       int minutes)
{
  secured_client.setInsecure();
  String message = "🚨 *HALLWAY PASS VIOLATION* 🚨\n\n";
  message += "Class: *Altair 12 - ICT*\n";
  message += " *" + studentName + "*\n";
  message += "Gender: *" + gender + "*\n";
  message += "Destination: *" + destination + "*\n";
  message += "Time Limit: " + String(minutes) + " minutes\n\n";
  message += "The student failed to return the card within the given time.";

  if (bot.sendMessage(CHAT_ID, message, "Markdown"))
    Serial.println("✅ Telegram sent.");
  else
    Serial.println("❌ Telegram failed.");
}

// Returns true when the pass card is back in ITS OWN slot.
static bool passCardReturned(SystemConfig &config,
                             bool femalePresent, bool malePresent)
{
  if (config.slotPassCardGender == "Female")
    return femalePresent;
  if (config.slotPassCardGender == "Male")
    return malePresent;
  // Unknown origin — require both to be safe.
  return femalePresent && malePresent;
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  DisplayOutputs::get().initialize();
  Inputs::get().initializeInputs();

  // Sync time. Real time is required for secure SSL connections.
  Serial.println("[NTP] Syncing time...");
  configTime(0, 0, "pool.ntp.org", "time.google.com");

  unsigned long ntpStart = millis();
  const unsigned long NTP_TIMEOUT = 15000UL;

  time_t now = time(nullptr);

  while (now < 24 * 3600 && millis() - ntpStart < NTP_TIMEOUT)
  {
    delay(250);
    now = time(nullptr);
  }

  if (now >= 24 * 3600)
  {
    Serial.println("[NTP] Time synchronized.");
  }
  else
  {
    Serial.println("[NTP] Sync failed or timed out.");
    Serial.println("[NTP] Continuing without synchronized time.");
  }

  DisplayOutputs::get().lcdPrint("HWPass Guard", "Choose card.");
  SystemConfig::get().currentState = DisplayState::SCAN_PASS;
}

void loop()
{
  SystemConfig &config = SystemConfig::get();
  // Keep the web server responsive.
  DisplayOutputs::get().handleClient();

  // ==========================================
  // SLOT RACE TRACKING / 30-SECOND TIMEOUT
  // ==========================================
  enum class RemovedSlot
  {
    NONE,
    FEMALE,
    MALE
  };

  static bool initialized = false;
  static bool previousFemalePresent = false;
  static bool previousMalePresent = false;
  static bool timerActive = false;
  static bool timeoutLockout = false;
  static bool guardMessageDisplayed = false;
  static bool passValidated = false; // true once a pass card has been accepted

  static RemovedSlot firstRemovedSlot = RemovedSlot::NONE;

  static unsigned long removalStartedAt = 0;
  static unsigned long lastSensorCheck = 0;

  const unsigned long SENSOR_INTERVAL = 1000UL; // 1 second
  const unsigned long SCAN_TIMEOUT = 30000UL;   // 30 seconds

  bool femalePresent = previousFemalePresent;
  bool malePresent = previousMalePresent;

  // ==========================================
  // SKIP SLOT TRACKING DURING COUNTDOWN and if the card is valid.
  // (student is legitimately out on a pass)
  // ==========================================
  if (config.currentState != DisplayState::COUNTDOWN && !passValidated)
  {
    // ==========================================
    // READ SENSORS EVERY 1 SECOND
    // LOW = CARD PRESENT
    // HIGH = SLOT EMPTY
    // ==========================================
    if (!initialized || millis() - lastSensorCheck >= SENSOR_INTERVAL)
    {
      lastSensorCheck = millis();

      femalePresent = (digitalRead(FEMALE_SLOT) == LOW);
      malePresent = (digitalRead(MALE_SLOT) == LOW);

      Serial.print("[SLOT] Female: ");
      Serial.print(femalePresent ? "PRESENT" : "EMPTY");
      Serial.print(" | Male: ");
      Serial.println(malePresent ? "PRESENT" : "EMPTY");

      if (!initialized)
      {
        initialized = true;
        previousFemalePresent = femalePresent;
        previousMalePresent = malePresent;

        Serial.println("[INIT] Sensor states initialized.");
      }
      else
      {
        bool femaleJustRemoved = previousFemalePresent && !femalePresent;
        bool maleJustRemoved = previousMalePresent && !malePresent;

        // Detect which slot was emptied first.
        if (firstRemovedSlot == RemovedSlot::NONE)
        {
          if (femaleJustRemoved && maleJustRemoved)
          {
            Serial.println("[RACE] Both slots emptied between checks.");
          }
          else if (femaleJustRemoved)
          {
            firstRemovedSlot = RemovedSlot::FEMALE;
            removalStartedAt = millis();
            timerActive = true;
            config.slotPassCardGender = "Female";

            DisplayOutputs::get().lcdPrint("Removed HWPass:", "Tap within 30s");
            Serial.println("[RACE] Female slot emptied first.");
            Serial.println("[TIMER] 30-second authentication timer started.");
          }
          else if (maleJustRemoved)
          {
            firstRemovedSlot = RemovedSlot::MALE;
            removalStartedAt = millis();
            timerActive = true;
            config.slotPassCardGender = "Male";

            DisplayOutputs::get().lcdPrint("Removed HWPass:", "Tap within 30s");
            Serial.println("[RACE] Male slot emptied first.");
            Serial.println("[TIMER] 30-second authentication timer started.");
          }
        }

        // Reset only when BOTH cards are back.
        if (femalePresent && malePresent)
        {
          if (firstRemovedSlot != RemovedSlot::NONE || timeoutLockout)
          {
            Serial.println("[RACE] Both cards returned. System reset.");
            DisplayOutputs::get().lcdPrint("Reseated both.", "Proceding.");
            delay(2000);
          }

          firstRemovedSlot = RemovedSlot::NONE;
          timerActive = false;
          timeoutLockout = false;
          guardMessageDisplayed = false;
        }

        previousFemalePresent = femalePresent;
        previousMalePresent = malePresent;
      }
    }

    // Use the most recent sensor readings between checks.
    femalePresent = previousFemalePresent;
    malePresent = previousMalePresent;

    // ==========================================
    // AUTHENTICATION TIMEOUT
    // ==========================================
    if (timerActive &&
        (unsigned long)(millis() - removalStartedAt) >= SCAN_TIMEOUT)
    {
      timerActive = false;
      timeoutLockout = true;

      Serial.println("[REVOKE] Authentication request expired.");
      Serial.println("[LOCKOUT] Return both cards to their slots.");

      DisplayOutputs::get().lcdPrint("Revoked: Reseat", "to proceed...");

      Inputs::get().beepRevokedRequest(); // Beep when revoked.
    }

    // ==========================================
    // LOCKOUT
    // Block all processing until both cards return.
    // ==========================================
    if (timeoutLockout)
    {
      return;
    }

    // ==========================================
    // HALLWAY GUARD LCD
    // Display only once while both cards are present.
    // ==========================================
    if (femalePresent && malePresent)
    {
      if (!guardMessageDisplayed)
      {
        DisplayOutputs::get().lcdPrint("HWPass Guard", "Choose card.");
        guardMessageDisplayed = true;

        Serial.println("[LCD] Hallway Guard displayed.");
      }
    }
    else
    {
      guardMessageDisplayed = false;
    }
  }

  // ==========================================
  // STATE 1: SCAN PASS CARD
  // ==========================================
  if (config.currentState == DisplayState::SCAN_PASS)
  {
    // Allow authentication as soon as either slot is empty.
    if (femalePresent && malePresent)
    {
      return;
    }

    if (Inputs::get().scanCard())
    {
      if (Inputs::get().isPassCardValid())
      {
        timerActive = false;
        firstRemovedSlot = RemovedSlot::NONE;

        config.currentState = DisplayState::HOME;

        DisplayOutputs::get().lcdPrint("Request allowed.", "Scan QRCode");

        Serial.println("[AUTH] Pass card authorized.");
        passValidated = true; // Track
      }
      else
      {
        DisplayOutputs::get().lcdPrint("Access Denied.", "Use HWPass card");

        Serial.println("[AUTH] Invalid HWPass card.");
        passValidated = false; // Track
      }

      Inputs::get().haltCard();
    }

    return;
  }

  // ==========================================
  // STATE 2: SCAN STUDENT ID
  // ==========================================
  if (config.currentState == DisplayState::SCAN_ID)
  {
    if (Inputs::get().scanCard())
    {
      if (Inputs::get().isStudentIDValid())
      {
        config.startTimer();
        config.currentState = DisplayState::COUNTDOWN;

        DisplayOutputs::get().lcdPrint("ID Verified!", "Starting timer.");

        delay(1500);

        DisplayOutputs::get().lcdPrint("Return.", "The card on time.");

        Serial.print("[ID] Verified: ");
        Serial.println(config.studentName);
      }
      else
      {
        DisplayOutputs::get().lcdPrint("ID Failed.", "Wrong Gender/ID.");

        Serial.println("[ID] Verification failed.");
      }

      Inputs::get().haltCard();
    }

    return;
  }

  // ==========================================
  // STATE 3: COUNTDOWN / RETURN PASS CARD
  // ==========================================
  if (config.currentState == DisplayState::COUNTDOWN)
  {
    if (Inputs::get().scanCard())
    {
      if (Inputs::get().isPassCardValid())
      {
        config.currentState = DisplayState::SUCCESS;

        DisplayOutputs::get().lcdPrint("Pass Recorded.", "Thank you!");

        Serial.println("[PASS] Card returned successfully.");

        Inputs::get().haltCard();

        delay(3000);

        config.currentState = DisplayState::SCAN_PASS;
        config.studentName = "";
        config.passCardGender = "";

        passValidated = false; // <-- reset so slot tracking resumes
        guardMessageDisplayed = false;

        // Enter returning guard instead of jumping to SCAN_PASS
        config.currentState = DisplayState::RETURN_CARD;

        DisplayOutputs::get().lcdPrint("Return card to", "its slot.");

        return;
      }

      Inputs::get().haltCard();
    }

    if (config.getSecondsRemaining() <= 0)
    {
      config.currentState = DisplayState::ERROR;

      DisplayOutputs::get().lcdPrint("VIOLATION!", "Return card now.");

      Serial.println("[VIOLATION] Time limit exceeded.");

      sendTelegramAlert(config.studentName, config.passCardGender,
                        config.selectedDestination, config.timeLimitMinutes);

      delay(5000);

      config.currentState = DisplayState::SCAN_PASS;
      config.studentName = "";
      config.passCardGender = "";

      passValidated = false; // <-- reset so slot tracking resumes
      guardMessageDisplayed = false;
      // Enter returning guard instead of jumping to SCAN_PASS
      config.currentState = DisplayState::RETURN_CARD;

      DisplayOutputs::get().lcdPrint("Return card to", "its slot.");
    }

    return;
  }

  // ==========================================
  // STATE 4: RETURNING GUARD
  // Wait until the pass card is back in its own slot
  // before re-enabling the hallway guard loop.
  // ==========================================
  if (config.currentState == DisplayState::RETURN_CARD)
  {
    static unsigned long lastReturnCheck = 0;
    const unsigned long RETURN_CHECK_INTERVAL = 1000UL;

    if (millis() - lastReturnCheck >= RETURN_CHECK_INTERVAL)
    {
      lastReturnCheck = millis();

      bool femaleNow = (digitalRead(FEMALE_SLOT) == LOW);
      bool maleNow = (digitalRead(MALE_SLOT) == LOW);

      Serial.print("[RETURN] Female: ");
      Serial.print(femaleNow ? "PRESENT" : "EMPTY");
      Serial.print(" | Male: ");
      Serial.println(maleNow ? "PRESENT" : "EMPTY");

      if (passCardReturned(config, femaleNow, maleNow))
      {
        Serial.println("[RETURN] Card returned to correct slot. Resuming.");

        DisplayOutputs::get().lcdPrint("Card returned.", "Resuming...");

        delay(1500);

        // Fully reset session
        config.studentName = "";
        config.passCardGender = "";
        config.slotPassCardGender = "";
        passValidated = false;
        guardMessageDisplayed = false;
        firstRemovedSlot = RemovedSlot::NONE;
        timerActive = false;
        timeoutLockout = false;

        // Sync the cached sensor snapshot so the slot-tracking block
        // doesn't immediately re-detect a removal/return edge.
        previousFemalePresent = femaleNow;
        previousMalePresent = maleNow;
        lastSensorCheck = millis();

        config.currentState = DisplayState::SCAN_PASS;

        DisplayOutputs::get().lcdPrint("HWPass Guard", "Choose card.");
      }
      else
      {
        // Keep informing the user which slot is missing.
        if (config.slotPassCardGender == "Female")
          DisplayOutputs::get().lcdPrint("Return HWPass", "to FEMALE slot.");
        else if (config.slotPassCardGender == "Male")
          DisplayOutputs::get().lcdPrint("Return HWPass", "to MALE slot.");
        else
          DisplayOutputs::get().lcdPrint("Return HWPass", "to its slot.");
      }
    }

    return; // Block all other states until the card is back
  }
}
