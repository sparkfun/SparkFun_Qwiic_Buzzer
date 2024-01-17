/******************************************************************************
  nvm.h
  
  By Pete Lewis @SparkFun Electronics
  Jan 2024

  Based on original code by the following:
  Nathan Seidle and Fischer Moseley and Priyanka Makin
  SparkFun Electronics
  Date: July 31st, 2019

  This file defines the locations in EEPROM where the configuration data is to
  be stored.

  SPDX-License-Identifier: MIT

  Copyright (c) 2023 SparkFun Electronics

  Distributed as-is; no warranty is given.
******************************************************************************/

#pragma once

//Location in EEPROM for each thing we want to store between power cycles
enum eepromLocations {
  kSfeQwiicBuzzerEepromLocationI2cAddress = 0x00,     // Device's address (uint8_t)
  kSfeQwiicBuzzerEepromLocationToneFrequency = 0x01,  // (uint16_t)
  kSfeQwiicBuzzerEepromLocationVolume = 0x03,         // (uint8_t)
  kSfeQwiicBuzzerEepromLocationDuration = 0x04,       // (uint16_t)
};
