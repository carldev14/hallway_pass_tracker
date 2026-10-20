#include "core/inputs.h"
#include <SPI.h>

Inputs &Inputs::get() {
  static Inputs instance;
  return instance;
}

void Inputs::initializeInputs() {
  // Start the standard SPI bus for the RFID reader
  SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, RFID_SS_PIN);

  // Initialize the RFID reader
  rfid_.PCD_Init();
  Serial.println("RFID Reader Initialized.");
}

bool Inputs::scanCard() {
  // Small delay to ensure stable reading
  delay(1);

  if (!rfid_.PICC_IsNewCardPresent())
    return false;
  if (!rfid_.PICC_ReadCardSerial())
    return false;
  return true;
}

bool Inputs::isAuthorized() {
  // Compare the scanned UID with the authorized UID in SystemConfig
  return (memcmp(rfid_.uid.uidByte, SystemConfig::get().authorizedHallwayPass,
                 4) == 0);
}

byte *Inputs::getScannedUID() { return rfid_.uid.uidByte; }

void Inputs::haltCard() {
  rfid_.PICC_HaltA();
  rfid_.PCD_StopCrypto1();
}
