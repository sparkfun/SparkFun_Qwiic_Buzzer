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

  //updates all the LED variables, and resets the pulseValues if necessary
  void update(struct memoryMap* map) {
    //check if any of the values are different
    bool different = false;
    if(map->buzzerVolume != volume) different = true; 
    if(map->buzzerToneFrequency != toneFrequency) different = true; 
    
    //if they are different, calculate new values and then reset everything
    if(different) {
      volume = map->buzzerVolume;
      toneFrequency = map->buzzerToneFrequency;
    }
  }
  
  void pulse(uint8_t buzzerPin) {
    //Pulse LED on/off based on settings
    //Brightness will increase with each cycle based on granularity value (5)
    //To get to max brightness (207) we will need ceil(207/5*2) LED adjustments = 83
    //At each discrete adjustment the LED will spend X amount of time at a brightness level
    //Time spent at this level is calc'd by taking total time (1000 ms) / number of adjustments / up/down (2) = 12ms per step

      tone(buzzerPin, toneFrequency);
      if(volume > 0)
      {
        digitalWrite(8, HIGH);
      } else {
        digitalWrite(8, LOW);
        noTone(buzzerPin);
      }
      return;
  }
};
