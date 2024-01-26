/******************************************************************************
    sfeQwiicBuzzerFirmware.h

    By Pete Lewis @SparkFun Electronics
    Jan 2024

    Based on original code by the following:
    Nathan Seidle and Fischer Moseley and Priyanka Makin
    SparkFun Electronics
    Date: July 31st, 2019

    This file defines the QwiicBuzzerFirmware class, which provides an easy interface for
    handling how the BUZZER behaves, in addition to storing configuration variables.

    SPDX-License-Identifier: MIT

    Copyright (c) 2023 SparkFun Electronics

    Distributed as-is; no warranty is given.
******************************************************************************/

#pragma once

#include <Arduino.h>

class QwiicBuzzerFirmware
{
public:
    /// @brief Set the GPIO pins used to control volume BJTs
    /// @param volPin0 GPIO pin used to control BJT for lowest volume setting
    /// @param volPin1 GPIO pin used to control BJT for mid-high volume setting
    /// @param volPin2 GPIO pin used to control BJT for mid-low volume setting
    /// @param volPin3 GPIO pin used to control BJT for highest volume setting
    /// @param statPin GPIO pin used to control the Status LED
    /// @param buzzPin GPIO pin used to control the Buzzer (high side BJT)
    void setupPins(uint8_t volPin0, uint8_t volPin1, uint8_t volPin2, uint8_t volPin3, uint8_t statPin, uint8_t buzzPin);

    /// @brief Updates all the QwiicBuzzerFirmware class variables, and resets if necessary
    /// @param map memoryMap struct containing all qwiic buzzer register data
    void updateFromMap(struct memoryMap *map);

    /// @brief Checks it there is any more duration to continue buzzing through
    /// @return True if there still is duration left, false if we are done
    bool checkDuration();

    /// @brief Resets everything to an "off" state
    /// @param map memoryMap struct containing all qwiic buzzer register data
    void reset(struct memoryMap *map);

    /// @brief A way to get the instance variable buzzerActiveFlag status
    /// @return True if buzzer is active, false if it is not
    bool active();

    /// @brief A way to get the instance variable duration
    /// @return duration
    bool duration();

private:
    // variables imported from registerMap, for use in this class
    uint8_t _volume;                   // Volume of buzzer
    uint16_t _toneFrequency;           // Frequency of the buzzer
    uint16_t _duration;                // Milliseconds, zero = forever
    uint32_t _buzzerStartTime;    // Start time of a new buzz, only useful if duration is used
    bool _buzzerActiveFlag = false; // actual local status of buzzer
    uint8_t _volumePin0;
    uint8_t _volumePin1;
    uint8_t _volumePin2;
    uint8_t _volumePin3;
    uint8_t _statusLedPin;
    uint8_t _buzzerPin;
};

/// @brief Location in EEPROM for each thing we want to store between power cycles
enum eepromLocations
{
    kSfeQwiicBuzzerEepromLocationI2cAddress = 0x00,    // Device's address (uint8_t)
    kSfeQwiicBuzzerEepromLocationToneFrequency = 0x01, // (uint16_t)
    kSfeQwiicBuzzerEepromLocationVolume = 0x03,        // (uint8_t)
    kSfeQwiicBuzzerEepromLocationDuration = 0x04,      // (uint16_t)
};

/// @brief defines the memoryMap struct, which acts as the pseudo register map
///  of the Qwiic Buzzer. It also serves as an easy way to access variables
///  and manipulate the state of the device.
///  During I2C transactions, the memoryMap object is wrapped as a collection of
///  bytes. The byte that the user is interested in (either to read or write) is
///  selected with a register pointer. For instance, if the user sets the pointer
///  to 0x03, they will be addressing the 4th uint8_t sized object in this struct.
///  In this case, that would be the buzzerToneFrequencyMSB register!
struct memoryMap
{
    // Buzzer Status/Configuration                       Register Address
    uint8_t id;            // 0x00
    uint8_t firmwareMinor; // 0x01
    uint8_t firmwareMajor; // 0x02

    uint8_t buzzerToneFrequencyMSB; // 0x03
    uint8_t buzzerToneFrequencyLSB; // 0x04
    uint8_t buzzerVolume;           // 0x05
    uint8_t buzzerDurationMSB;      // 0x06
    uint8_t buzzerDurationLSB;      // 0x07
    uint8_t buzzerActive;           // 0x08
    uint8_t saveSettings;           // 0x09

    // Device Configuration
    uint8_t i2cAddress; // 0x0A
};