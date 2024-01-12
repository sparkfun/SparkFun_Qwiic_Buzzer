/******************************************************************************
  interrupts.ino
  Fischer Moseley @ SparkFun Electronics
  Original Creation Date: July 31, 2019

  This file contains the interrupt routines that are triggered upon an I2C write from
  master (receiveEvent), or an I2C read (requestEvent). 
  These ISRs modify the registerMap state variable, and sometimes
  set a flag (updateFlag) that updates things in the main loop.

  This code is beerware; if you see me (or any other SparkFun employee) at the
  local, and you've found our code helpful, please buy us a round!

  Distributed as-is; no warranty is given.
******************************************************************************/

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