/******************************************************************************
nvm.h
Fischer Moseley @ SparkFun Electronics
Original Creation Date: July 31, 2019

This file defines the locations in EEPROM where the configuration data is to
be stored.

This code is beerware; if you see me (or any other SparkFun employee) at the
local, and you've found our code helpful, please buy us a round!

Distributed as-is; no warranty is given.
******************************************************************************/

//Location in EEPROM for each thing we want to store between power cycles
enum eepromLocations {
  LOCATION_I2C_ADDRESS = 0x00,            //Device's address (uint8_t)
  LOCATION_BUZZER_TONE_FREQUENCY = 0x01,  // (uint16_t)
  LOCATION_BUZZER_VOLUME = 0x03,          // (uint8_t)
  LOCATION_BUZZER_DURATION = 0x04,        // (uint16_t)
};
