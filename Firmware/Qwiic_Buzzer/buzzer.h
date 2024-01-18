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
        uint8_t volume = 3;                 // Volume of buzzer
        uint16_t toneFrequency = 2730;      // Frequency of the buzzer
        uint16_t duration = 0;              // Milliseconds, zero = forever
        unsigned long buzzerStartTime = 0;  // Start time of a new buzz, only useful if duration is used
        boolean buzzerActiveFlag = false;   // actual local status of buzzer
        uint8_t volumePin0 = 0;
        uint8_t volumePin1 = 0;
        uint8_t volumePin2 = 0;
        uint8_t volumePin3 = 0;
        const uint8_t statusLedPin = 0;

        /// @brief Set the GPIO pins used to control volume BJTs
        /// @param pin0 GPIO pin used to control BJT for lowest volume setting
        /// @param pin1 GPIO pin used to control BJT for mid-high volume setting
        /// @param pin2 GPIO pin used to control BJT for mid-low volume setting
        /// @param pin3 GPIO pin used to control BJT for highest volume setting
        void setupVolumePins(uint8_t pin0, uint8_t pin1, uint8_t pin2, uint8_t pin3)
        {
            volumePin0 = pin0;
            volumePin1 = pin1;
            volumePin2 = pin2;
            volumePin3 = pin3;

            // all volume settings output low - OFF
            pinMode(volumePin0, OUTPUT);
            pinMode(volumePin1, OUTPUT);
            pinMode(volumePin2, OUTPUT);
            pinMode(volumePin3, OUTPUT);
            digitalWrite(volumePin0, LOW);
            digitalWrite(volumePin1, LOW);
            digitalWrite(volumePin2, LOW);
            digitalWrite(volumePin3, LOW);
        }

        /// @brief Updates all the QwiicBuzzer class variables, and resets if necessary
        /// @param map memoryMap struct containing all qwiic buzzer register data
        /// @param buzzerPin GPIO pin used to activate the on-board physical buzzer        
        void updateFromMap(struct memoryMap* map, uint8_t buzzerPin) 
        {
            //check if any of the values are different
            bool different = false;
            if(map->buzzerVolume != volume) different = true; 

            uint16_t mapToneFrequency = 0x00; //used to store temp complete uint16_t from maps high/low bytes.
            uint8_t freqMSB = map->buzzerToneFrequencyMSB;
            uint8_t freqLSB = map->buzzerToneFrequencyLSB;
            mapToneFrequency |= freqLSB;
            mapToneFrequency |= (freqMSB << 8);
            if(mapToneFrequency != toneFrequency) different = true; 

            uint16_t mapDuration_uint16 = 0x00; //used to store temp complete uint16_t from maps high/low bytes.
            uint8_t durationMSB = map->buzzerDurationMSB;
            uint8_t durationLSB = map->buzzerDurationLSB;
            mapDuration_uint16 |= durationLSB;
            mapDuration_uint16 |= (durationMSB << 8);
            if(mapDuration_uint16 != toneFrequency) different = true;     

            //if they are different, update class variables
            if(different) 
            {
                volume = map->buzzerVolume;
                toneFrequency = mapToneFrequency;
                duration = mapDuration_uint16;
            }

            // if the previous statusFlag of the buzzer means it sitting "off",
            // then we want to check and see if the user just made it active,
            // and then turn on the buzzer (but only once).
            uint8_t mapBuzzerActive = map->buzzerActive;

            if ( (mapBuzzerActive == 0x01) && (buzzerActiveFlag == false) ) // this means we were off, and now the user wants to turn it on.
            {
                if(duration > 0)
                {
                    if(toneFrequency != 0) // to avoid clicking sounds if the user wants to play a "rest" note (frequency set to 0)
                    {
                        tone(buzzerPin,toneFrequency, duration); 
                    }
                    buzzerStartTime = millis(); // used to know when to turn off the GND connection on the buzzer (aka the volume setting to zero).
                }
                else
                {
                    if(toneFrequency != 0) // to avoid clicking sounds if the user wants to play a "rest" note (frequency set to 0)
                    {
                        tone(buzzerPin,toneFrequency); 
                    }
                }

                // volume control
                if(toneFrequency != 0) // to avoid clicking sounds if the user wants to play a "rest" note (frequency set to 0)
                {
                    // first, turn off all volumes, then turn on the only desired setting
                    digitalWrite(volumePin0, LOW);
                    digitalWrite(volumePin1, LOW);
                    digitalWrite(volumePin2, LOW);
                    digitalWrite(volumePin3, LOW);
                    switch(volume) 
                    {
                        case 0: // off
                            // do nothing, they are already turned off above
                            break;
                        case 1: // setting 1 - aka "quietest" or "user"
                            digitalWrite(volumePin3, HIGH);
                            break;
                        case 2: // setting 2 - aka "mid-low"
                            digitalWrite(volumePin2, HIGH);
                            break;
                        case 3: // setting 3 - aka "mid-high"
                            digitalWrite(volumePin1, HIGH);
                            break;
                        case 4: // setting 4 - aka "loudest"
                            digitalWrite(volumePin0, HIGH);
                            break;                              
                    }
                }
                pinMode(statusLedPin, OUTPUT);
                digitalWrite(statusLedPin, HIGH);
                buzzerActiveFlag = true;
            }
            else if ( (mapBuzzerActive == 0x00) && (buzzerActiveFlag == true) ) // this means we are currently on, and the user wants to turn it off
            {
                reset(map, buzzerPin);
            }
        }
        
        /// @brief Checks it there is any more duration to continue buzzing through
        /// @return True if there still is duration left, false if we are done
        bool checkDuration()
        {
            if (millis() - buzzerStartTime > duration) // we've surpassed duration, time to turn off
            {
                return false;
            }
            return true;
        }

        /// @brief Resets everything to an "off" state
        /// @param map memoryMap struct containing all qwiic buzzer register data
        /// @param buzzerPin GPIO pin used to activate the on-board physical buzzer 
        void reset(struct memoryMap* map, uint8_t buzzerPin)
        {
            // Stop the tone.
            // Note, this could have already been stopped by tone's duration
            noTone(buzzerPin);        

            // Disable GND side connections
            digitalWrite(volumePin0, LOW);     
            digitalWrite(volumePin1, LOW);
            digitalWrite(volumePin2, LOW);
            digitalWrite(volumePin3, LOW);

            // Clear the map->buzzerActive register
            map->buzzerActive = 0x00; 

            // Turn off the status LED
            digitalWrite(statusLedPin, LOW);
            pinMode(statusLedPin, INPUT);

            // Update class variable of buzzerActiveFlag to reflect reality
            buzzerActiveFlag = false;
        }
};


