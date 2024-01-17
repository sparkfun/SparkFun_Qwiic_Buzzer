/******************************************************************************
  registers.h
  
  By Pete Lewis @SparkFun Electronics
  Jan 2024

  Based on original code by the following:
  Nathan Seidle and Fischer Moseley and Priyanka Makin
  SparkFun Electronics
  Date: July 31st, 2019

  This file defines the memoryMap struct, which acts as the pseudo register map
  of the Qwiic Buzzer. It also serves as an easy way to access variables
  and manipulate the state of the device.

  During I2C transactions, the memoryMap object is wrapped as a collection of
  bytes. The byte that the user is interested in (either to read or write) is
  selected with a register pointer. For instance, if the user sets the pointer
  to 0x03, they will be addressing the 4th uint8_t sized object in this struct.
  In this case, that would be the buzzerToneFrequencyMSB register!

  SPDX-License-Identifier: MIT

  Copyright (c) 2023 SparkFun Electronics

  Distributed as-is; no warranty is given.
******************************************************************************/

#pragma once

struct memoryMap {
    //Buzzer Status/Configuration                       Register Address
    uint8_t id;                                             // 0x00
    uint8_t firmwareMinor;                                  // 0x01
    uint8_t firmwareMajor;                                  // 0x02

    uint8_t buzzerToneFrequencyMSB;                         // 0x03
    uint8_t buzzerToneFrequencyLSB;                         // 0x04
    uint8_t buzzerVolume;                                   // 0x05
    uint8_t buzzerDurationMSB;                              // 0x06
    uint8_t buzzerDurationLSB;                              // 0x07
    uint8_t buzzerActive;                                   // 0x08
    uint8_t saveSettings;                                   // 0x09

    //Device Configuration
    uint8_t i2cAddress;                                     // 0x0A
};
