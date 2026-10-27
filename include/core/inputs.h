#ifndef INPUTS_H
#define INPUTS_H

#include "system/config.h"
#include <Arduino.h>
#include <MFRC522.h>

class Inputs {
public:
  static Inputs &get();
  void initializeInputs();
  bool scanCard();
  void haltCard();

  // Identification helpers
  bool isPassCardValid();
  bool isStudentIDValid();
  void beepSuccess();
  void beepError();
  void beepRevokedRequest();

  bool isCardInSlot(); // Verify if card is exists

private:
  MFRC522 rfid_{RFID_SS_PIN, RFID_RST_PIN};
  Inputs() = default;
  ~Inputs() = default;
  Inputs(const Inputs &) = delete;
  Inputs &operator=(const Inputs &) = delete;
};

#endif // INPUTS_H
