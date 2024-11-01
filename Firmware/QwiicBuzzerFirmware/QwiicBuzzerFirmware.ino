/*
  An I2C based Buzzer
  By Pete Lewis @SparkFun Electronics
  Jan 2024

  Based on original code by the following:
  Nathan Seidle and Fischer Moseley and Priyanka Makin
  SparkFun Electronics
  Date: July 31st, 2019

  SPDX-License-Identifier: MIT

  Copyright (c) 2023 SparkFun Electronics

  Qwiic Buzzer is an I2C based buzzer that accepts commands to turn on/off the buzzer.
  It also can set it's frequency, duration and volume.

  There is also an accompanying Arduino Library located here:
  https://github.com/sparkfun/SparkFun_Qwiic_Buzzer_Arduino_Library

  Feel like supporting our work? Buy a board from SparkFun!
  https://www.sparkfun.com/products/14641

  To install support for ATtiny84 in Arduino IDE: https://github.com/SpenceKonde/ATTinyCore/blob/master/Installation.md
  This core is installed from the Board Manager menu
  This core has built in support for I2C S/M and serial
  If you have Dave Mellis' ATtiny installed you may need to remove it from \Users\xx\AppData\Local\Arduino15\Packages

  To support 400kHz I2C communication reliably ATtiny84 needs to run at 8MHz. This requires user to
  click on 'Burn Bootloader' before code is loaded.

  Library Inclusion:
  Wire.h        Used for interfacing with the I2C hardware for responding to I2C events.
  EEPROM.h      Used for interfacing with the onboard EEPROM for storing and retrieving settings.
  sfeQwiicBuzzerFirmware.h      Used for configuring the behavior of the onboard BUZZER

  avr/sleep.h             Needed for sleep_mode which saves power on the ATTiny
  avr/power.hardware      Needed for powering down peripherals such as the ADC/TWI and Timers on the ATTiny
*/

#include <Wire.h>
#include <EEPROM.h>
#include "sfeQwiicBuzzerFirmware.h"

#include <avr/sleep.h> //Needed for sleep_mode
#include <avr/power.h> //Needed for powering down perihperals such as the ADC/TWI and Timers

#define kSfeQwiicBuzzerDeviceID 0x5E
#define kSfeQwiicBuzzerFirmwareVersionMajor 0x01 // Firmware Version. Helpful for tech support.
#define kSfeQwiicBuzzerFirmwareVersionMinor 0x00

#define kSfeQwiicBuzzerDefaultI2cAddress 0x34

#define kSfeQwiicBuzzerResonantFrequency 2730

uint8_t sfeQwiicBuzzerOldI2cAddress;

bool previousTriggerPinState = true;            // used to detect transitions from high to low on trigger pin.
uint8_t triggerPinLowCounter = 0;               // debounce trigger pin status counter
uint8_t triggerPinHighCounter = 0;              // debounce trigger pin status counter
#define kSfeQwiicBuzzerTriggerDebounceAmount 10 // the amount of digitalReads required to recognise a Trigger pin status

// Hardware connections
#if defined(__AVR_ATmega328P__)
// For developement on an Uno
const uint8_t volumePin0 = 3;
const uint8_t volumePin1 = 4;
const uint8_t volumePin2 = 5;
const uint8_t volumePin3 = 6;
const uint8_t buzzerPin = 9; // PWM
const uint8_t statusLedPin = 8;
const uint8_t triggerPin = 2;

#elif defined(__AVR_ATtiny84__)
const uint8_t volumePin0 = 9;
const uint8_t volumePin1 = 10;
const uint8_t volumePin2 = 1;
const uint8_t volumePin3 = 2;
const uint8_t buzzerPin = 7; // for use with tone()
const uint8_t statusLedPin = 0;
const uint8_t triggerPin = 5;
#endif

/// @brief Initialize the Qwiic Buzzer register map with default values
memoryMap registerMap{
  kSfeQwiicBuzzerDeviceID,             // id
  kSfeQwiicBuzzerFirmwareVersionMinor, // firmwareMinor
  kSfeQwiicBuzzerFirmwareVersionMajor, // firmwareMajor
  0x00,                                // buzzerToneFrequencyMSB
  0x00,                                // buzzerToneFrequencyLSB
  0x00,                                // buzzerVolume
  0x00,                                // buzzerDurationMSB
  0x00,                                // buzzerDurationLSB
  0x00,                                // buzzerActive
  0x00,                                // saveSettings
  0x00,                                // i2cAddress
};

/// @brief Set permissions on each register member (0=read-only, 1=read-write)
memoryMap protectionMap = {
  0x00, // id
  0x00, // firmwareMinor
  0x00, // firmwareMajor
  0xFF, // buzzerToneFrequencyMSB
  0xFF, // buzzerToneFrequencyLSB
  0x07, // buzzerVolume
  0xFF, // buzzerDurationMSB
  0xFF, // buzzerDurationLSB
  0x01, // buzzerActive
  0x01, // saveSettings
  0xFF, // i2cAddress
};

// Cast 32bit address of the object registerMap with uint8_t so we can increment the pointer
uint8_t *registerPointer = (uint8_t *)&registerMap;
uint8_t *protectionPointer = (uint8_t *)&protectionMap;

/// @brief Gets set when user writes an address. We then serve the spot the user requested.
volatile uint8_t registerNumber;

/// @brief Goes true when we receive new bytes from user. Causes things to update in main loop.
volatile boolean updateFlag = true;

/// @brief Initialize the onboard Buzzer
QwiicBuzzerFirmware buzzer;

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void setup(void)
{
  // Setup GPIO pins in the class
  buzzer.setupPins(volumePin0, volumePin1, volumePin2, volumePin3, statusLedPin, buzzerPin);

  // GPIO with internal pullup, goes low when user connects to GND
  pinMode(triggerPin, INPUT_PULLUP);

  /// @brief Used to measure how long the trigger pin has been held low. Must be
  /// held low for 1000ms to engage a factory reset
  int factoryResetPinLowCounter = 0;

  // Check for factory reset
  // If the user connects the TRIGGER pin low on boot-up, and continues to hold
  // it low for a full second, then perform a factory reset (write all settings
  // EEPROM to "fresh" values of 0xFF.
  while (digitalRead(triggerPin) == LOW)
  {
    delay(1);
    factoryResetPinLowCounter += 1;
    if (factoryResetPinLowCounter > 1000)
    {
      factoryReset();
      while (digitalRead(triggerPin) == LOW)
        ; // wait for trigger to go high
      break;
    }
  }

  // Disable ADC
  ADCSRA = 0;

  // Disable Brown-Out Detect
  MCUCR = bit(BODS) | bit(BODSE);
  MCUCR = bit(BODS);

  // Power down various bits of hardware to lower power usage
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();

#if defined(__AVR_ATmega328P__)

  for (int x = 0; x < 100; x++)
  {
    EEPROM.put(x, 0xFF);
  }
  Serial.begin(115200);
  Serial.println("Qwiic Buzzer");
  Serial.print("Address: 0x");
  Serial.println(registerMap.i2cAddress, HEX);
  Serial.print("Device ID: 0x");
  Serial.println(registerMap.id, HEX);

#endif

  // Load all system settings from EEPROM
  readSystemSettings(&registerMap);

  // Update Buzzer variables
  buzzer.updateFromMap(&registerMap);

  // Determine the I2C address we should be using
  // and begin listening on I2C bus
  startI2C(&registerMap);

  // Set old I2C address, so we can keep track for later, during a change.
  sfeQwiicBuzzerOldI2cAddress = registerMap.i2cAddress;
}

void loop(void)
{
  // Check to see if the I2C Address has been updated by software, set the appropriate address type flag
  if (sfeQwiicBuzzerOldI2cAddress != registerMap.i2cAddress)
  {
    sfeQwiicBuzzerOldI2cAddress = registerMap.i2cAddress;
  }

  // If buzzer is active and there is duration set, checkDuration
  // This will keep the buzzer buzzing until duration has been completed
  if (buzzer.active() && (registerMap.buzzerDurationLSB || registerMap.buzzerDurationMSB))
  {
    if (buzzer.checkDuration() == false)
    {
      buzzer.reset(&registerMap);
    }
  }

  // Check for updateFlag.
  // This is set inside interrupts (quickly, in order to keep the ISR zippy)
  if (updateFlag == true)
  {
    // Record anything new to EEPROM (for example, new I2C address)
    // Note, to save other settings (like frequency, duration or volume) the
    // user must also set the "saveSettings" register bit.
    // It can take ~3.4ms to write a byte to EEPROM so we do that here instead of in an interrupt
    recordSystemSettings(&registerMap);

    // Update settings from register map to actively used local variables
    buzzer.updateFromMap(&registerMap);

    // clear flag
    updateFlag = false;
  }

  // Check if TRIGGER has transitioned from high to low, and buzz!
  if ((digitalRead(triggerPin) == LOW) && (previousTriggerPinState == true))
  {
    // debounce
    triggerPinLowCounter += 1;

    if (triggerPinLowCounter > kSfeQwiicBuzzerTriggerDebounceAmount)
    {
      // Reset everything
      buzzer.reset(&registerMap);

      // set the map->buzzerActive register
      // This will be "caught" in updateFromMap and actually turn on the buzzer
      registerMap.buzzerActive = 0x01;

      updateFlag = true;               // this will cause an update in main loop
      triggerPinLowCounter = 0;        // reset debounce counter
      previousTriggerPinState = false; // keep track of status
    }
  }
  else if ((previousTriggerPinState == false) && (digitalRead(triggerPin) == HIGH)) // user has release trigger pin
  {
    // debounce
    triggerPinHighCounter += 1;

    if (triggerPinHighCounter > kSfeQwiicBuzzerTriggerDebounceAmount)
    {
      // If duration is set to zero, then we want this transition from LOW to HIGH to stop buzzer
      // Otherwise, duration will play out and stop the buzzer for us.
      if (buzzer.duration() == 0)
      {
        // Reset everything
        buzzer.reset(&registerMap);

        updateFlag = true; // this will cause an update in main loop
      }
      triggerPinHighCounter = 0;      // reset debounce counter
      previousTriggerPinState = true; // keep track of status
    }
  }

  // Stop everything and go to sleep. Wake up if I2C event occurs.
  sleep_mode();
}

/// @brief Update slave I2C address to what's configured with registerMap.i2cAddress
/// @param map memoryMap struct containing all qwiic buzzer register data
void startI2C(memoryMap *map)
{
  uint8_t address;

  // if the value is legal, then set it
  if (map->i2cAddress > 0x07 && map->i2cAddress < 0x78)
    address = map->i2cAddress;

  // if the value is illegal, default to the default I2C address for our platform
  else
    address = kSfeQwiicBuzzerDefaultI2cAddress;

  // save new address to the register map
  map->i2cAddress = address;

  // reconfigure Wire instance
  Wire.end();          // stop I2C on old address
  Wire.begin(address); // rejoin the I2C bus on new address

  // The connections to the interrupts are severed when a Wire.begin occurs, so here we reattach them
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
}

/// @brief Reads the current system settings from EEPROM
/// If anything looks weird, reset setting to default value
/// @param map memoryMap struct containing all qwiic buzzer register data
void readSystemSettings(memoryMap *map)
{
  // Read what I2C address we should use
  EEPROM.get(kSfeQwiicBuzzerEepromLocationI2cAddress, map->i2cAddress);
  if (map->i2cAddress == 255)
  {
    map->i2cAddress = kSfeQwiicBuzzerDefaultI2cAddress; // By default, we listen for kSfeQwiicBuzzerDefaultI2cAddress
    EEPROM.put(kSfeQwiicBuzzerEepromLocationI2cAddress, map->i2cAddress);
  }

  // Error check I2C address we read from EEPROM
  if (map->i2cAddress < 0x08 || map->i2cAddress > 0x77)
  {
    // User has set the address out of range
    // Go back to defaults
    map->i2cAddress = kSfeQwiicBuzzerDefaultI2cAddress;
    EEPROM.put(kSfeQwiicBuzzerEepromLocationI2cAddress, map->i2cAddress);
  }

  // Tone Frequency
  uint16_t toneFrequency = 0; // used to store temp complete uint16_t from maps high/low bytes.

  EEPROM.get(kSfeQwiicBuzzerEepromLocationToneFrequency, toneFrequency);
  if (toneFrequency == 0xFFFF)
  {
    toneFrequency = kSfeQwiicBuzzerResonantFrequency; // Default to resonant frequency (2.73KHz)
    EEPROM.put(kSfeQwiicBuzzerEepromLocationToneFrequency, toneFrequency);
  }
  // extract MSB and LSB from complete uint16_t
  // put it into the registerMap for use everywhere
  uint8_t toneFrequencyMSB = ((toneFrequency & 0xFF00) >> 8);
  uint8_t toneFrequencyLSB = (toneFrequency & 0x00FF);
  map->buzzerToneFrequencyMSB = toneFrequencyMSB;
  map->buzzerToneFrequencyLSB = toneFrequencyLSB;

  // Duration
  uint16_t toneDuration = 0; // used to store temp complete uint16_t from maps high/low bytes.

  EEPROM.get(kSfeQwiicBuzzerEepromLocationDuration, toneDuration);
  if (toneDuration == 0xFFFF)
  {
    toneDuration = 0; // Default to zero (forever)
    EEPROM.put(kSfeQwiicBuzzerEepromLocationDuration, toneDuration);
  }
  // extract MSB and LSB from complete uint16_t
  // put it into the registerMap for use everywhere
  uint8_t toneDurationMSB = ((toneDuration & 0xFF00) >> 8);
  uint8_t toneDurationLSB = (toneDuration & 0x00FF);
  map->buzzerDurationMSB = toneDurationMSB;
  map->buzzerDurationLSB = toneDurationLSB;

  // Volume
  EEPROM.get(kSfeQwiicBuzzerEepromLocationVolume, map->buzzerVolume);
  if (map->buzzerVolume == 0xFF)
  {
    map->buzzerVolume = 4; // Default to full
    EEPROM.put(kSfeQwiicBuzzerEepromLocationVolume, map->buzzerVolume);
  }
}

/// @brief Record current settings in registerMap to EEPROM (only if different)
/// @param map memoryMap struct containing all qwiic buzzer register data
void recordSystemSettings(memoryMap *map)
{
  // I2C address is byte
  byte i2cAddr;

  // Error check the current I2C address
  if (map->i2cAddress >= 0x08 && map->i2cAddress <= 0x77)
  {
    // Address is valid

    // Read the value currently in EEPROM. If it's different from the memory map then record the memory map value to EEPROM.
    EEPROM.get(kSfeQwiicBuzzerEepromLocationI2cAddress, i2cAddr);
    if (i2cAddr != map->i2cAddress)
    {
      EEPROM.put(kSfeQwiicBuzzerEepromLocationI2cAddress, (byte)map->i2cAddress);
      startI2C(map); // Determine the I2C address we should be using and begin listening on I2C bus
    }
  }
  else
  {
    EEPROM.get(kSfeQwiicBuzzerEepromLocationI2cAddress, i2cAddr);
    map->i2cAddress = i2cAddr; // Return to original address
  }

  // if user has set bit 0 in the ""saveSettings" register, then record settings to EEPROM.
  if (map->saveSettings == 1)
  {
    uint16_t mapToneFrequency = map->buzzerToneFrequencyLSB | (map->buzzerToneFrequencyMSB << 8);
    EEPROM.put(kSfeQwiicBuzzerEepromLocationToneFrequency, mapToneFrequency);

    uint16_t mapDuration = map->buzzerDurationLSB | (map->buzzerDurationMSB << 8);
    EEPROM.put(kSfeQwiicBuzzerEepromLocationDuration, mapDuration);

    EEPROM.put(kSfeQwiicBuzzerEepromLocationVolume, map->buzzerVolume);

    // clear "saveSettings" bit, so it only happens once
    map->saveSettings = 0x00;
  }
}

/// @brief I2C interrupt, called when data has been received from an I2C controller
/// @param numberOfBytesReceived The number of bytes that a I2C controller has sent
void receiveEvent(int numberOfBytesReceived)
{
  // Get the memory map offset from the user
  registerNumber = Wire.read();

  // Begin recording the following incoming bytes to the temp memory map
  // starting at the registerNumber (the first byte received)
  for (uint8_t registerOffset = 0; registerOffset < numberOfBytesReceived - 1; registerOffset++)
  {
    // We might record it, we might throw it away
    uint8_t temp = Wire.read();

    if ((registerOffset + registerNumber) < sizeof(memoryMap))
    {
      // Clense the incoming byte against the read only protected bits
      // Store the result into the register map
      *(registerPointer + registerNumber + registerOffset) &= ~*(protectionPointer + registerNumber + registerOffset);       // Clear this register if needed
      *(registerPointer + registerNumber + registerOffset) |= temp & *(protectionPointer + registerNumber + registerOffset); // Or in the user's request (clensed against protection bits)
    }
  }

  if (numberOfBytesReceived > 1) // only update if we received a valid amount
  {
    updateFlag = true; // Update in the main loop
  }
}

/// @brief I2C interrupt, called when data has been requested by an I2C controller
/// The interrupt will respond with bytes starting from the last byte the user sent to us
/// This will write the entire contents of the register map struct starting from
/// the register the user requested, and when it reaches the end the I2C-controller
/// will read 0xFFs.
/// @param numberOfBytesReceived The number of bytes that a I2C controller has sent
void requestEvent()
{
  Wire.write((registerPointer + registerNumber), sizeof(memoryMap) - registerNumber);
}

/// @brief Perform a factory reset.
/// This will write all values in settings EEPROM to "fresh" values of 0xFF.
void factoryReset()
{
  // write settings EEPROM locations to 0xFF
  EEPROM.put(kSfeQwiicBuzzerEepromLocationI2cAddress, 0xFF);
  EEPROM.put(kSfeQwiicBuzzerEepromLocationToneFrequency, 0xFFFF);
  EEPROM.put(kSfeQwiicBuzzerEepromLocationVolume, 0xFF);
  EEPROM.put(kSfeQwiicBuzzerEepromLocationDuration, 0xFFFF);

  // blink the status LED three times to indicate that factory reset has completed.
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(statusLedPin, HIGH);
    delay(100);
    digitalWrite(statusLedPin, LOW);
    delay(100);
  }
}