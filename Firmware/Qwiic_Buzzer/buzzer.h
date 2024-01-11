/******************************************************************************
led.h
Fischer Moseley @ SparkFun Electronics
Original Creation Date: July 31, 2019

This file defines the BUZZERconfig struct, which provides an easy interface for
handling how the LED behaves, in addition to storing configuration variables.

On the Qwiic Button, this LED is built into the button itself! 
On the Qwiic Switch, a pin for an external LED is provided on the 0.1" header
located on the bottom of the switch.

This code is beerware; if you see me (or any other SparkFun employee) at the
local, and you've found our code helpful, please buy us a round!

Distributed as-is; no warranty is given.
******************************************************************************/

//manages LED operations, stores configuration variables internally and provides an easy interface to updating the LED's status and blinking it
struct BUZZERconfig {

  //variables imported from registerMap
  uint8_t volume = 0;  //Volume of buzzer
  uint16_t toneFrequency = 0; //Total pulse cycle in ms, does not include off time. LED pulse disabled if zero.
  uint16_t duration = 0; // Miliseconds the note will continue to buzz, zero = forever
  unsigned long buzzerStartTime = 0; // Start time of a new buzz, only useful if duration is used
  boolean buzzerActiveFlag = false; // actual local status of buzzer

  //updates all the LED variables, and resets the pulseValues if necessary
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

    //if they are different, calculate new values and then reset everything
    if(different) {
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
        tone(buzzerPin,toneFrequency, duration);
        buzzerStartTime = millis(); // used to know when to turn off the GND connection on the buzzer (aka the volume setting to zero).
      }
      else
      {
        tone(buzzerPin,toneFrequency);
      }
      digitalWrite(8, HIGH);
      buzzerActiveFlag = true;
    }
    else if ( (mapBuzzerActive == 0x00) && (buzzerActiveFlag == true) ) // this means we are currently on, and the user wants to turn it off
    {
      noTone(buzzerPin);
      digitalWrite(8, LOW);
      buzzerActiveFlag = false;
    }
  }
  
  // checkDuration
  // This checks to see if there is any duration left
  // When duration runs out, this will do four things:
  // 1. call noTone()
  // 2. disable GND connection side bjts
  // 3. clear the map->buzzerActive register
  // 4. update the global buzzerActiveFlag to false

  // Note, this is only ever called in from the main loop if the buzzer is currently active and duration is non-zero.

  void checkDuration(struct memoryMap* map, uint8_t buzzerPin)
  {
    if (millis() - buzzerStartTime > duration) // we've surpassed duration, time to turn off
    {
      noTone(buzzerPin);        // stop the tone - although this has already most likely been stopped by its own duration feature
      digitalWrite(8, LOW);     // disable GND side connections
      map->buzzerActive = 0x00; // clear the map->buzzerActive register
      buzzerActiveFlag = false; // update the global buzzerActiveFlag to false
    }
  }
};


