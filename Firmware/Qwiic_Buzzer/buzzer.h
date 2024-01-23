/******************************************************************************
  buzzer.h

  By Pete Lewis @SparkFun Electronics
  Jan 2024

  Based on original code by the following:
  Nathan Seidle and Fischer Moseley and Priyanka Makin
  SparkFun Electronics
  Date: July 31st, 2019

  This file defines the QwiicBuzzer class, which provides an easy interface for
  handling how the BUZZER behaves, in addition to storing configuration variables.

  SPDX-License-Identifier: MIT

  Copyright (c) 2023 SparkFun Electronics

  Distributed as-is; no warranty is given.
******************************************************************************/

#pragma once

#include <Arduino.h>
#include "nvm.h"
#include "registers.h"

class QwiicBuzzer 
{
    public:
        // variables imported from registerMap, for use in this class
        uint8_t _volume;                 // Volume of buzzer
        uint16_t _toneFrequency;         // Frequency of the buzzer
        uint16_t _duration;              // Milliseconds, zero = forever
        unsigned long _buzzerStartTime;  // Start time of a new buzz, only useful if duration is used
        boolean _buzzerActiveFlag = false;   // actual local status of buzzer
        uint8_t _volumePin0;
        uint8_t _volumePin1;
        uint8_t _volumePin2;
        uint8_t _volumePin3;
        uint8_t _statusLedPin;
        uint8_t _buzzerPin;

        /// @brief Set the GPIO pins used to control volume BJTs
        /// @param volPin0 GPIO pin used to control BJT for lowest volume setting
        /// @param volPin1 GPIO pin used to control BJT for mid-high volume setting
        /// @param volPin2 GPIO pin used to control BJT for mid-low volume setting
        /// @param volPin3 GPIO pin used to control BJT for highest volume setting
        /// @param statPin GPIO pin used to control the Status LED
        /// @param buzzPin GPIO pin used to control the Buzzer (high side BJT)
        void setupPins(uint8_t volPin0, uint8_t volPin1, uint8_t volPin2, uint8_t volPin3, uint8_t statPin, uint8_t buzzPin)
        {
            _volumePin0 = volPin0;
            _volumePin1 = volPin1;
            _volumePin2 = volPin2;
            _volumePin3 = volPin3;
            _statusLedPin = statPin;
            _buzzerPin = buzzPin;            

            // all volume settings output low - OFF
            pinMode(_volumePin0, OUTPUT);
            pinMode(_volumePin1, OUTPUT);
            pinMode(_volumePin2, OUTPUT);
            pinMode(_volumePin3, OUTPUT);
            digitalWrite(_volumePin0, LOW);
            digitalWrite(_volumePin1, LOW);
            digitalWrite(_volumePin2, LOW);
            digitalWrite(_volumePin3, LOW);

            pinMode(_statusLedPin, OUTPUT);
            digitalWrite(_statusLedPin, LOW);

            pinMode(_buzzerPin, OUTPUT);
            digitalWrite(_buzzerPin, LOW);
        }

        /// @brief Updates all the QwiicBuzzer class variables, and resets if necessary
        /// @param map memoryMap struct containing all qwiic buzzer register data       
        void updateFromMap(struct memoryMap* map)
        {
            //check if any of the values are different, and update class variables
            bool different = false;
            if(map->buzzerVolume != _volume) 
            {
                _volume = map->buzzerVolume;
                different = true;
            }

            uint16_t mapToneFrequency = 0x00; //used to store temp complete uint16_t from maps high/low bytes.
            uint8_t freqMSB = map->buzzerToneFrequencyMSB;
            uint8_t freqLSB = map->buzzerToneFrequencyLSB;
            mapToneFrequency |= freqLSB;
            mapToneFrequency |= (freqMSB << 8);
            if(mapToneFrequency != _toneFrequency)
            {
                _toneFrequency = mapToneFrequency;
                different = true; 
            }

            uint16_t mapDuration_uint16 = 0x00; //used to store temp complete uint16_t from maps high/low bytes.
            uint8_t durationMSB = map->buzzerDurationMSB;
            uint8_t durationLSB = map->buzzerDurationLSB;
            mapDuration_uint16 |= durationLSB;
            mapDuration_uint16 |= (durationMSB << 8);
            if(mapDuration_uint16 != _duration)
            {
                _duration = mapDuration_uint16;
                different = true;
            }

            // if the previous statusFlag of the buzzer means it sitting "off",
            // then we want to check and see if the user just made it active,
            // and then turn on the buzzer (but only once).
            uint8_t mapBuzzerActive = map->buzzerActive;

            if ( (mapBuzzerActive == 0x01) && ( (_buzzerActiveFlag == false) || (different == true) ) ) // this means we were off, and now the user wants to turn it on.
            {
                if(_duration > 0) // Duration is used, and we're starting a new buzz, so need to reset _buzzerStartTime.
                {
                    _buzzerStartTime = millis(); // used to know when to turn off the GND connection on the buzzer (aka the volume setting to zero).
                }

                if(_toneFrequency != 0) // to avoid clicking sounds if the user wants to play a "rest" note (frequency set to 0)
                {
                    tone(_buzzerPin,_toneFrequency); 
                }
                else // User has changed frequency to 0, (aka the "rest" note).
                {
                    noTone();
                }

                // volume control
                if(_toneFrequency != 0) // to avoid clicking sounds if the user wants to play a "rest" note (frequency set to 0)
                {
                    // first, turn off all volumes, then turn on the only desired setting
                    digitalWrite(_volumePin0, LOW);
                    digitalWrite(_volumePin1, LOW);
                    digitalWrite(_volumePin2, LOW);
                    digitalWrite(_volumePin3, LOW);
                    switch(_volume) 
                    {
                        case 1: // setting 1 - aka "quietest" or "user"
                            digitalWrite(_volumePin3, HIGH);
                            break;
                        case 2: // setting 2 - aka "mid-low"
                            digitalWrite(_volumePin2, HIGH);
                            break;
                        case 3: // setting 3 - aka "mid-high"
                            digitalWrite(_volumePin1, HIGH);
                            break;
                        case 4: // setting 4 - aka "loudest"
                            digitalWrite(_volumePin0, HIGH);
                            break;          
                        default: // off
                            // do nothing, they are already turned off above
                            break;                    
                    }
                }
                digitalWrite(_statusLedPin, HIGH);
                _buzzerActiveFlag = true;
            }
            else if ( (mapBuzzerActive == 0x00) && (_buzzerActiveFlag == true) ) // this means we are currently on, and the user wants to turn it off
            {
                reset(map);
            }
        }
        
        /// @brief Checks it there is any more duration to continue buzzing through
        /// @return True if there still is duration left, false if we are done
        bool checkDuration()
        {
            return (millis() - _buzzerStartTime < _duration); // we've surpassed duration, time to turn off
        }

        /// @brief Resets everything to an "off" state
        /// @param map memoryMap struct containing all qwiic buzzer register data
        void reset(struct memoryMap* map)
        {
            // Stop the tone.
            // Note, this could have already been stopped by tone's duration
            noTone(_buzzerPin);        

            // Disable GND side connections
            digitalWrite(_volumePin0, LOW);     
            digitalWrite(_volumePin1, LOW);
            digitalWrite(_volumePin2, LOW);
            digitalWrite(_volumePin3, LOW);

            // Clear the map->buzzerActive register
            map->buzzerActive = 0x00; 

            // Turn off the status LED
            digitalWrite(_statusLedPin, LOW);

            // Update class variable of _buzzerActiveFlag to reflect reality
            _buzzerActiveFlag = false;
        }

        /// @brief A way to get the instance variable buzzerActiveFlag status
        /// @return True if buzzer is active, false if it is not
        bool active()
        {
            return _buzzerActiveFlag;
        }        
};


