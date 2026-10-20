/**
 * @file core/inputs.h
 * @brief Input handling — RFID reader only.
 */
#ifndef INPUTS_H
#define INPUTS_H

#include "system/config.h"
#include <Arduino.h>
#include <MFRC522.h>

class Inputs {
public:
  // ==========================================
  // SINGLETON
  // ==========================================
  static Inputs &get();

  /**
   * @brief Initializes SPI and the RFID reader.
   */
  void initializeInputs();

  // ==========================================
  // RFID INPUTS
  // ==========================================

  /**
   * @brief Checks if a new RFID card is present and reads its UID.
   * @return true if a card was successfully read, false otherwise.
   */
  bool scanCard();

  /**
   * @brief Checks if the most recently scanned card is authorized.
   * @return true if authorized, false otherwise.
   */
  bool isAuthorized();

  /**
   * @brief Gets the raw UID byte array of the currently scanned card.
   * @return Pointer to the 4-byte UID array.
   */
  byte *getScannedUID();

  /**
   * @brief Halts the current RFID card to prevent multiple reads.
   */
  void haltCard();

private:
  // ==========================================
  // MEMBER VARIABLES
  // ==========================================
  MFRC522 rfid_{RFID_SS_PIN, RFID_RST_PIN};

  // ==========================================
  // SINGLETON ENFORCEMENT
  // ==========================================
  Inputs() = default;
  ~Inputs() = default;
  Inputs(const Inputs &) = delete;
  Inputs &operator=(const Inputs &) = delete;
};

#endif // INPUTS_H
