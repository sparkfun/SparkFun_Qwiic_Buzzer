/******************************************************************************
led.h
Fischer Moseley @ SparkFun Electronics
Original Creation Date: July 31, 2019

This file defines the BUZZERconfig struct, which provides an easy interface for
handling how the BUZZER behaves, in addition to storing configuration variables.

This code is beerware; if you see me (or any other SparkFun employee) at the
local, and you've found our code helpful, please buy us a round!

Distributed as-is; no warranty is given.
******************************************************************************/

struct BUZZERconfig {

  //variables imported from registerMap
  uint8_t volume = 0;  //Volume of buzzer
  uint16_t toneFrequency = 2730; //Total pulse cycle in ms, does not include off time. LED pulse disabled if zero.
  uint16_t duration = 0; // Miliseconds the note will continue to buzz, zero = forever
  unsigned long buzzerStartTime = 0; // Start time of a new buzz, only useful if duration is used
  boolean buzzerActiveFlag = false; // actual local status of buzzer
  uint8_t volumePin0 = 0;
  uint8_t volumePin1 = 0;
  uint8_t volumePin2 = 0;
  uint8_t volumePin3 = 0;
  const uint8_t statusLedPin = 0;

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

      // volume control
      // first, turn off all volumes, then turn on the only desired setting
      digitalWrite(volumePin0, LOW);
      digitalWrite(volumePin1, LOW);
      digitalWrite(volumePin2, LOW);
      digitalWrite(volumePin3, LOW);
      switch(volume) {
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
      pinMode(statusLedPin, OUTPUT);
      digitalWrite(statusLedPin, HIGH);
      buzzerActiveFlag = true;
    }
    else if ( (mapBuzzerActive == 0x00) && (buzzerActiveFlag == true) ) // this means we are currently on, and the user wants to turn it off
    {
      noTone(buzzerPin);
      digitalWrite(volumePin0, LOW);
      digitalWrite(volumePin1, LOW);
      digitalWrite(volumePin2, LOW);
      digitalWrite(volumePin3, LOW);
      digitalWrite(statusLedPin, LOW);
      pinMode(statusLedPin, INPUT);
      buzzerActiveFlag = false;
    }
  }
  
  // checkDuration
  // This checks to see if there is any duration left
  // When duration runs out, this will do four things:
  // 1. call noTone()
  // 2. disable all GND connection side bjts
  // 3. clear the map->buzzerActive register
  // 4. update the buzzerActiveFlag to false

  // Note, this is only ever called in from the main loop if the buzzer is currently active and duration is non-zero.

  void checkDuration(struct memoryMap* map, uint8_t buzzerPin)
  {
    if (millis() - buzzerStartTime > duration) // we've surpassed duration, time to turn off
    {
      noTone(buzzerPin);        // stop the tone - although this has already been stopped by its own duration feature
      digitalWrite(volumePin0, LOW);     // disable GND side connections
      digitalWrite(volumePin1, LOW);
      digitalWrite(volumePin2, LOW);
      digitalWrite(volumePin3, LOW);
      map->buzzerActive = 0x00; // clear the map->buzzerActive register
      digitalWrite(statusLedPin, LOW);
      pinMode(statusLedPin, INPUT);
      buzzerActiveFlag = false; // update the global buzzerActiveFlag to false
    }
  }

  void reset(struct memoryMap* map, uint8_t buzzerPin)
  {
      noTone(buzzerPin);        // stop the tone - although this has already been stopped by its own duration feature
      digitalWrite(volumePin0, LOW);     // disable GND side connections
      digitalWrite(volumePin1, LOW);
      digitalWrite(volumePin2, LOW);
      digitalWrite(volumePin3, LOW);
      map->buzzerActive = 0x00; // clear the map->buzzerActive register
      digitalWrite(statusLedPin, LOW);
      pinMode(statusLedPin, INPUT);
      buzzerActiveFlag = false; // update the global buzzerActiveFlag to false    
  }
};


