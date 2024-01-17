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
  nvm.h         Used for defining the storage locations in non-volatile memory (EEPROM) to store and retrieve settings from.
  registers.h   Used for defining a memoryMap object that serves as the register map for the device.
  buzzer.h      Used for configuring the behavior of the onboard BUZZER

  avr/sleep.h             Needed for sleep_mode which saves power on the ATTiny
  avr/power.hardware      Needed for powering down peripherals such as the ADC/TWI and Timers on the ATTiny
*/

#include <Wire.h>
#include <EEPROM.h>
#include "buzzer.h"

#include <avr/sleep.h> //Needed for sleep_mode
#include <avr/power.h> //Needed for powering down perihperals such as the ADC/TWI and Timers

#define kSfeQwiicBuzzerDeviceID 0x5D
#define kSfeQwiicBuzzerFirmwareVersionMajor 0x01 //Firmware Version. Helpful for tech support.
#define kSfeQwiicBuzzerFirmwareVersionMinor 0x03

#define kSfeQwiicBuzzerDefaultI2cAddress 0x34

uint8_t sfeQwiicBuzzerOldI2cAddress;

//Hardware connections
#if defined(__AVR_ATmega328P__)
//For developement on an Uno
const uint8_t volumePin0 = 3;
const uint8_t volumePin1 = 4;
const uint8_t volumePin2 = 5;
const uint8_t volumePin3 = 6;
const uint8_t buzzerPin = 9; //PWM
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
memoryMap registerMap {
  kSfeQwiicBuzzerDeviceID,                  // id
  kSfeQwiicBuzzerFirmwareVersionMinor,      // firmwareMinor
  kSfeQwiicBuzzerFirmwareVersionMajor,      // firmwareMajor
  0x0A,                                     // buzzerToneFrequencyMSB
  0xAA,                                     // buzzerToneFrequencyLSB
  0x00,                                     // buzzerVolume  
  0x00,                                     // buzzerDurationMSB  
  0x00,                                     // buzzerDurationLSB  
  0x00,                                     // buzzerActive  
  0x00,                                     // saveSettings
  kSfeQwiicBuzzerDefaultI2cAddress,         // i2cAddress
};

/// @brief Set permissions on each register member (0=read-only, 1=read-write)
memoryMap protectionMap = {
  0x00,                                     // id
  0x00,                                     // firmwareMinor
  0x00,                                     // firmwareMajor
  0xFF,                                     // buzzerToneFrequencyMSB
  0xFF,                                     // buzzerToneFrequencyLSB
  0xFF,                                     // buzzerVolume  
  0xFF,                                     // buzzerDurationMSB  
  0xFF,                                     // buzzerDurationLSB 
  0xFF,                                     // buzzerActive  
  0xFF,                                     // saveSettings
  0xFF,                                     // i2cAddress
};

//Cast 32bit address of the object registerMap with uint8_t so we can increment the pointer
uint8_t *registerPointer = (uint8_t *)&registerMap;
uint8_t *protectionPointer = (uint8_t *)&protectionMap;

/// @brief Gets set when user writes an address. We then serve the spot the user requested.
volatile uint8_t registerNumber;

/// @brief Goes true when we receive new bytes from user. Causes things to update in main loop.
volatile boolean updateFlag = true;

/// @brief Initialize the onboard Buzzer
BUZZERconfig onboardBUZZER;

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void setup(void)
{
  // setup volume pins in buzzer config struct
  onboardBUZZER.setupVolumePins(volumePin0, volumePin1, volumePin2, volumePin3);

  pinMode(buzzerPin, OUTPUT);

  pinMode(triggerPin, INPUT_PULLUP); //GPIO with internal pullup, goes low when user connects to GND

  //Disable ADC
  ADCSRA = 0;

  //Disable Brown-Out Detect
  MCUCR = bit(BODS) | bit(BODSE);
  MCUCR = bit(BODS);

  //Power down various bits of hardware to lower power usage
  //set_sleep_mode(SLEEP_MODE_PWR_DOWN); //May turn off millis
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();

#if defined(__AVR_ATmega328P__)

  for (int x = 0; x < 100; x++) {
    EEPROM.put(x, 0xFF);
  }

  Serial.begin(115200);
  Serial.println("Qwiic Buzzer");
  Serial.print("Address: 0x");
  Serial.println(registerMap.i2cAddress, HEX);
  Serial.print("Device ID: 0x");
  Serial.println(registerMap.id, HEX);

#endif

  readSystemSettings(&registerMap); //Load all system settings from EEPROM

  onboardBUZZER.updateFromMap(&registerMap, buzzerPin); //update BUZZER variables
  startI2C(&registerMap);          //Determine the I2C address we should be using and begin listening on I2C bus
  sfeQwiicBuzzerOldI2cAddress = registerMap.i2cAddress;
  digitalWrite(statusLedPin, LOW); //turn off stat LED - this only comes on when we buzz
}

void loop(void)
{
  // Check to see if the I2C Address has been updated by software, set the appropriate address type flag
  if (sfeQwiicBuzzerOldI2cAddress != registerMap.i2cAddress)
  {
    sfeQwiicBuzzerOldI2cAddress = registerMap.i2cAddress;
  }
  
  // if buzzer is active and there is duration set
  if ((onboardBUZZER.buzzerActiveFlag == true) && (registerMap.buzzerDurationLSB || registerMap.buzzerDurationMSB) )
  {
    onboardBUZZER.checkDuration(&registerMap, buzzerPin);
  }

  if (updateFlag == true)
  {
    // Record anything new to EEPROM (for example, new I2C address)
    // Note, to save other settings (like frequency, duration or volume) the
    // user must also set the "saveSettings" register bit.
    // It can take ~3.4ms to write a byte to EEPROM so we do that here instead of in an interrupt
    recordSystemSettings(&registerMap);

    // Update settings from register map to actively used local variables
    onboardBUZZER.updateFromMap(&registerMap, buzzerPin);

    updateFlag = false; // clear flag
  }

  if (digitalRead(triggerPin) == LOW)
  {
    onboardBUZZER.reset(&registerMap, buzzerPin);
    registerMap.buzzerActive = 0x01; // set the map->buzzerActive register
    onboardBUZZER.updateFromMap(&registerMap, buzzerPin);

    while(digitalRead(triggerPin) == LOW) 
    {
      // if buzzer is active and there is duration set
      if ((onboardBUZZER.buzzerActiveFlag == true) && (registerMap.buzzerDurationLSB || registerMap.buzzerDurationMSB) )
      {
        onboardBUZZER.checkDuration(&registerMap, buzzerPin);
      }
      delay(1);
    }
    onboardBUZZER.reset(&registerMap, buzzerPin);
  }
  
  sleep_mode();             // Stop everything and go to sleep. Wake up if I2C event occurs.
}

//Update slave I2C address to what's configured with registerMap.i2cAddress and/or the address jumpers.
void startI2C(memoryMap *map)
{
  uint8_t address;


    //if the value is legal, then set it
    if (map->i2cAddress > 0x07 && map->i2cAddress < 0x78)
      address = map->i2cAddress;

    //if the value is illegal, default to the default I2C address for our platform
    else
      address = kSfeQwiicBuzzerDefaultI2cAddress;
  

  //save new address to the register map
  map->i2cAddress = address;

  //reconfigure Wire instance
  Wire.end();          //stop I2C on old address
  Wire.begin(address); //rejoin the I2C bus on new address

  //The connections to the interrupts are severed when a Wire.begin occurs, so here we reattach them
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
}

//Reads the current system settings from EEPROM
//If anything looks weird, reset setting to default value
void readSystemSettings(memoryMap *map)
{
  //Read what I2C address we should use
  EEPROM.get(kSfeQwiicBuzzerEepromLocationI2cAddress, map->i2cAddress);
  if (map->i2cAddress == 255)
  {
    map->i2cAddress = kSfeQwiicBuzzerDefaultI2cAddress; //By default, we listen for kSfeQwiicBuzzerDefaultI2cAddress
    EEPROM.put(kSfeQwiicBuzzerEepromLocationI2cAddress, map->i2cAddress);
  }

  //Error check I2C address we read from EEPROM
  if (map->i2cAddress < 0x08 || map->i2cAddress > 0x77)
  {
    //User has set the address out of range
    //Go back to defaults
    map->i2cAddress = kSfeQwiicBuzzerDefaultI2cAddress;
    EEPROM.put(kSfeQwiicBuzzerEepromLocationI2cAddress, map->i2cAddress);
  }

  uint16_t mapToneFrequency  = 0; //used to store temp complete uint16_t from maps high/low bytes.

  EEPROM.get(kSfeQwiicBuzzerEepromLocationToneFrequency, mapToneFrequency);
  if (mapToneFrequency == 0xFFFF)
  {
    mapToneFrequency = 2730; //Default to resonant frequency
    EEPROM.put(kSfeQwiicBuzzerEepromLocationToneFrequency, mapToneFrequency);
  }
  // extract MSB and LSB from complete uint16_t
  // put it into the registerMap for use everywhere
  uint8_t toneFrequencyMSB = ((mapToneFrequency & 0xFF00) >> 8 );
  uint8_t toneFrequencyLSB = (mapToneFrequency & 0x00FF);
  map->buzzerToneFrequencyMSB = toneFrequencyMSB;
  map->buzzerToneFrequencyLSB = toneFrequencyLSB;

  uint16_t mapDuration; //used to store temp complete uint16_t from maps high/low bytes.
  mapDuration |= map->buzzerDurationLSB;
  mapDuration |= (map->buzzerDurationMSB << 8);

  EEPROM.get(kSfeQwiicBuzzerEepromLocationDuration, mapDuration);
  if (mapDuration == 0xFFFF)
  {
    mapDuration = 0; //Default to none
    EEPROM.put(kSfeQwiicBuzzerEepromLocationDuration, mapDuration);
  }  

  EEPROM.get(kSfeQwiicBuzzerEepromLocationVolume, map->buzzerVolume);
  if (map->buzzerVolume == 0xFF)
  {
    map->buzzerVolume = 0; //Default to none
    EEPROM.put(kSfeQwiicBuzzerEepromLocationVolume, map->buzzerVolume);
  }
}

//If the current setting is different from that in EEPROM, update EEPROM
void recordSystemSettings(memoryMap *map)
{
  //I2C address is byte
  byte i2cAddr;

  //Error check the current I2C address
  if (map->i2cAddress >= 0x08 && map->i2cAddress <= 0x77)
  {
    //Address is valid

    //Read the value currently in EEPROM. If it's different from the memory map then record the memory map value to EEPROM.
    EEPROM.get(kSfeQwiicBuzzerEepromLocationI2cAddress, i2cAddr);
    if (i2cAddr != map->i2cAddress)
    {
      EEPROM.put(kSfeQwiicBuzzerEepromLocationI2cAddress, (byte)map->i2cAddress);
      startI2C(map); //Determine the I2C address we should be using and begin listening on I2C bus
    }
  }
  else
  {
    EEPROM.get(kSfeQwiicBuzzerEepromLocationI2cAddress, i2cAddr);
    map->i2cAddress = i2cAddr; //Return to original address
  }

  // if user has set the ""saveSettings" bit, then record settings to EEPROM.
  if(map->saveSettings == 0x01)
  {
    uint16_t mapToneFrequency = 0x00; //used to store temp complete uint16_t from maps high/low bytes.
    uint8_t freqMSB = map->buzzerToneFrequencyMSB; // get MSB from map
    uint8_t freqLSB = map->buzzerToneFrequencyLSB; // get MLB from map
    mapToneFrequency |= freqLSB; // combine MSB/LSM into temp complete
    mapToneFrequency |= (freqMSB << 8); // combine MSB/LSM into temp complete
    EEPROM.put(kSfeQwiicBuzzerEepromLocationToneFrequency, mapToneFrequency);

    uint16_t mapDuration; //used to store temp complete uint16_t from maps high/low bytes.
    mapDuration |= map->buzzerDurationLSB;
    mapDuration |= (map->buzzerDurationMSB << 8);
    EEPROM.put(kSfeQwiicBuzzerEepromLocationDuration, mapDuration);

    EEPROM.put(kSfeQwiicBuzzerEepromLocationVolume, map->buzzerVolume);

    // clear "saveSettings" bit, so it only happens once
    map->saveSettings = 0x00;
  }
}

//When Qwiic Buzzer receives data bytes from Master, this function is called as an interrupt
void receiveEvent(int numberOfBytesReceived) {
  registerNumber = Wire.read(); //Get the memory map offset from the user

  //Begin recording the following incoming bytes to the temp memory map
  //starting at the registerNumber (the first byte received)
  for (uint8_t x = 0 ; x < numberOfBytesReceived - 1 ; x++) {
    uint8_t temp = Wire.read(); //We might record it, we might throw it away

    if ( (x + registerNumber) < sizeof(memoryMap)) {
      //Clense the incoming byte against the read only protected bits
      //Store the result into the register map
      *(registerPointer + registerNumber + x) &= ~*(protectionPointer + registerNumber + x); //Clear this register if needed
      *(registerPointer + registerNumber + x) |= temp & *(protectionPointer + registerNumber + x); //Or in the user's request (clensed against protection bits)
    }
  }
  updateFlag = true; //Update in the main loop
}

//Respond to GET commands
//When Qwiic Buzzer gets a request for data from the user, this function is called as an interrupt
//The interrupt will respond with bytes starting from the last byte the user sent to us
//While we are sending bytes we may have to do some calculations
void requestEvent() {
  //This will write the entire contents of the register map struct starting from
  //the register the user requested, and when it reaches the end the master
  //will read 0xFFs.

  Wire.write((registerPointer + registerNumber), sizeof(memoryMap) - registerNumber);
}