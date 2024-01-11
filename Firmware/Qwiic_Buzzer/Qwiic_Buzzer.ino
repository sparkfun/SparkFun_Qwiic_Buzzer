/*
  An I2C based Buzzer
  By Pete Lewis @SparkFun Electronics
  Jan 2024

  Based on original code by the following:
  Nathan Seidle and Fischer Moseley and Priyanka Makin
  SparkFun Electronics
  Date: July 31st, 2019

  License: This code is public domain but you buy me a beer if you use this and
  we meet someday (Beerware license).

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

  PinChangeInterrupt.h    Nico Hoo's library for triggering an interrupt on a pin state change (either low->high or high->low)
  avr/sleep.h             Needed for sleep_mode which saves power on the ATTiny
  avr/power.hardware      Needed for powering down peripherals such as the ADC/TWI and Timers on the ATTiny
*/

#include <Wire.h>
#include <EEPROM.h>
#include "nvm.h"
#include "registers.h"
#include "buzzer.h"

#include "PinChangeInterrupt.h" //Nico Hood's library: https://github.com/NicoHood/PinChangeInterrupt/
//Used for pin change interrupts on ATtinys (encoder buzzer causes interrupt)
  /*** NOTE, PinChangeInterrupt library NEEDS a modification to work with this code.
  *** you MUST comment out this line: 
  *** https://github.com/NicoHood/PinChangeInterrupt/blob/master/src/PinChangeInterruptSettings.h#L228
  */

#include <avr/sleep.h> //Needed for sleep_mode
#include <avr/power.h> //Needed for powering down perihperals such as the ADC/TWI and Timers

#define DEVICE_ID 0x5D
#define FIRMWARE_MAJOR 0x01 //Firmware Version. Helpful for tech support.
#define FIRMWARE_MINOR 0x03

#define DEFAULT_I2C_ADDRESS 0x34

#define SOFTWARE_ADDRESS true
#define HARDWARE_ADDRESS false

uint8_t oldAddress;

//Hardware connections
#if defined(__AVR_ATmega328P__)
//For developement on an Uno
const uint8_t volumePin0 = 3;
const uint8_t volumePin1 = 4;
const uint8_t volumePin2 = 5;
const uint8_t volumePin3 = 6;
const uint8_t buzzerPin = 9; //PWM
const uint8_t statusLedPin = 8;
const uint8_t switchPin = 2;
const uint8_t interruptPin = 7; //pin is active-low, high-impedance when not triggered, goes low-impedance to ground when triggered

#elif defined(__AVR_ATtiny84__)
const uint8_t volumePin0 = 9;
const uint8_t volumePin1 = 10;
const uint8_t volumePin2 = 1;
const uint8_t volumePin3 = 2;
const uint8_t buzzerPin = 7; // for use with tone()
const uint8_t statusLedPin = 3;
const uint8_t switchPin = 8;
const uint8_t interruptPin = 0; //pin is active-low, high-impedance when not triggered, goes low-impedance to ground when triggered
#endif

//Global variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//These are the defaults for all settings

//Variables used in the I2C interrupt.ino file so we use volatile
memoryMap registerMap {
  DEVICE_ID,           // id
  FIRMWARE_MINOR,      // firmwareMinor
  FIRMWARE_MAJOR,      // firmwareMajor
  0x00,                // buzzerToneFrequencyMSB
  0x00,                // buzzerToneFrequencyLSB
  0x00,                // buzzerVolume  
  0x00,                // buzzerDurationMSB  
  0x00,                // buzzerDurationLSB  
  0x00,                // buzzerActive  
  0x00,                // saveSettings
  DEFAULT_I2C_ADDRESS, // i2cAddress
};

//This defines which of the registers are read-only (0) vs read-write (1)
memoryMap protectionMap = {
  0x00,       // id
  0x00,       // firmwareMinor
  0x00,       // firmwareMajor
  0xFF,       // buzzerToneFrequencyMSB
  0xFF,       // buzzerToneFrequencyLSB
  0xFF,       // buzzerVolume  
  0xFF,       // buzzerDurationMSB  
  0xFF,       // buzzerDurationLSB 
  0xFF,       // buzzerActive  
  0xFF,       // saveSettings
  0xFF,       // i2cAddress
};

//Cast 32bit address of the object registerMap with uint8_t so we can increment the pointer
uint8_t *registerPointer = (uint8_t *)&registerMap;
uint8_t *protectionPointer = (uint8_t *)&protectionMap;

volatile uint8_t registerNumber; //Gets set when user writes an address. We then serve the spot the user requested.

volatile boolean updateFlag = true; //Goes true when we receive new bytes from user. Causes LEDs and things to update in main loop.

BUZZERconfig onboardBUZZER; //init the onboard LED

#include "pitches.h"

// notes in the melody:
int melody[] = {
  NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  4, 8, 8, 4, 4, 4, 4, 4
};

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void setup(void)
{
  // setup volume pins in buzzer config struct
  onboardBUZZER.setupVolumePins(volumePin0, volumePin1, volumePin2, volumePin3);

  pinMode(buzzerPin, OUTPUT);

  pinMode(statusLedPin, OUTPUT); //No PWM
  digitalWrite(statusLedPin, 0);

  pinMode(switchPin, INPUT_PULLUP); //GPIO with internal pullup, goes low when button is pushed

#if defined(__AVR_ATmega328P__)
  pinMode(interruptPin, INPUT_PULLUP);     //High-impedance input until we have an int and then we output low. Pulled high with 10k with cuttable jumper.
#else
  pinMode(interruptPin, INPUT);     //High-impedance input until we have an int and then we output low. Pulled high with 10k with cuttable jumper.
#endif

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

#if defined(__AVR_ATmega328P__)
  //Debug values
  registerMap.buzzerVolume = 255;     //Max brightness
  registerMap.ledPulseGranularity = 1; //Amount to change LED at each step

  registerMap.buzzerToneFrequency = 500; //Total amount of cycle, does not include off time. LED pulse disabled if zero.
  registerMap.ledPulseOffTime = 500;   //Off time between pulses
#endif

  onboardBUZZER.updateFromMap(&registerMap, buzzerPin); //update LED variables, get ready for pulsing
  setupInterrupts();               //Enable pin change interrupts for I2C, switch, etc
  startI2C(&registerMap);          //Determine the I2C address we should be using and begin listening on I2C bus
  oldAddress = registerMap.i2cAddress;

  digitalWrite(statusLedPin, HIGH); //turn on the status LED to notify that we've setup everything properly

}

void loop(void)
{
  // Check to see if the I2C Address has been updated by software, set the appropriate address type flag
  if (oldAddress != registerMap.i2cAddress)
  {
    oldAddress = registerMap.i2cAddress;
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
      address = DEFAULT_I2C_ADDRESS;
  

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
  EEPROM.get(LOCATION_I2C_ADDRESS, map->i2cAddress);
  if (map->i2cAddress == 255)
  {
    map->i2cAddress = DEFAULT_I2C_ADDRESS; //By default, we listen for DEFAULT_I2C_ADDRESS
    EEPROM.put(LOCATION_I2C_ADDRESS, map->i2cAddress);
  }

  //Error check I2C address we read from EEPROM
  if (map->i2cAddress < 0x08 || map->i2cAddress > 0x77)
  {
    //User has set the address out of range
    //Go back to defaults
    map->i2cAddress = DEFAULT_I2C_ADDRESS;
    EEPROM.put(LOCATION_I2C_ADDRESS, map->i2cAddress);
  }

  uint16_t mapToneFrequency  = 0x00; //used to store temp complete uint16_t from maps high/low bytes.
  mapToneFrequency &= map->buzzerToneFrequencyLSB;
  mapToneFrequency &= (map->buzzerToneFrequencyMSB << 8);

  EEPROM.get(LOCATION_BUZZER_TONE_FREQUENCY, mapToneFrequency);
  if (mapToneFrequency == 0xFFFF)
  {
    mapToneFrequency = 0; //Default to none
    EEPROM.put(LOCATION_BUZZER_TONE_FREQUENCY, mapToneFrequency);
  }

  uint16_t mapDuration; //used to store temp complete uint16_t from maps high/low bytes.
  mapDuration &= map->buzzerDurationLSB;
  mapDuration &= (map->buzzerDurationMSB << 8);

  EEPROM.get(LOCATION_BUZZER_DURATION, mapDuration);
  if (mapDuration == 0xFFFF)
  {
    mapDuration = 0; //Default to none
    EEPROM.put(LOCATION_BUZZER_DURATION, mapDuration);
  }  

  EEPROM.get(LOCATION_BUZZER_VOLUME, map->buzzerVolume);
  if (map->buzzerVolume == 0xFF)
  {
    map->buzzerVolume = 0; //Default to none
    EEPROM.put(LOCATION_BUZZER_VOLUME, map->buzzerVolume);
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
    EEPROM.get(LOCATION_I2C_ADDRESS, i2cAddr);
    if (i2cAddr != map->i2cAddress)
    {
      EEPROM.put(LOCATION_I2C_ADDRESS, (byte)map->i2cAddress);
      startI2C(map); //Determine the I2C address we should be using and begin listening on I2C bus
    }
  }
  else
  {
    EEPROM.get(LOCATION_I2C_ADDRESS, i2cAddr);
    map->i2cAddress = i2cAddr; //Return to original address
  }

  // if user has set the ""saveSettings" bit, then record settings to EEPROM.
  if(map->saveSettings == 0x01)
  {
    uint16_t mapToneFrequency = 0x00; //used to store temp complete uint16_t from maps high/low bytes.
    mapToneFrequency |= map->buzzerToneFrequencyLSB;
    mapToneFrequency |= (map->buzzerToneFrequencyMSB << 8);
    EEPROM.put(LOCATION_BUZZER_TONE_FREQUENCY, mapToneFrequency);

    uint16_t mapDuration; //used to store temp complete uint16_t from maps high/low bytes.
    mapDuration &= map->buzzerDurationLSB;
    mapDuration &= (map->buzzerDurationMSB << 8);
    EEPROM.put(LOCATION_BUZZER_DURATION, mapDuration);

    EEPROM.put(LOCATION_BUZZER_VOLUME, map->buzzerVolume);

    // clear "saveSettings" bit, so it only happens once
    map->saveSettings = 0x00;
  }
}

void play_melody()
{
    // iterate over the notes of the melody:
  for (int thisNote = 0; thisNote < 8; thisNote++) {

    // to calculate the note duration, take one second divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(7, melody[thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(7);
  }
}
