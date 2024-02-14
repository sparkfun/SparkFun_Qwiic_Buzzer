In this section, we will highlight the hardware and pins that are broken out on the SparkFun Qwiic Buzzer.

<div style="text-align: center;">
  <table>
    <tr style="vertical-align:middle;">
     <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><a href="../assets/img/BOB-24474-Qwiic-Buzzer_Top_View.jpg"><img src="../assets/img/BOB-24474-Qwiic-Buzzer_Top_View.jpg" width="600px" height="600px" alt="Top View"></a></td>
     <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><a href="../assets/img/BOB-24474-Qwiic-Buzzer_Bottom_View.jpg"><img src="../assets/img/BOB-24474-Qwiic-Buzzer_Bottom_View.jpg" width="600px" height="600px" alt="Bottom View"></a></td>
    </tr>
    <tr style="vertical-align:middle;">
     <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><i>Top View</i></td>
     <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><i>Bottom View</i></td>
    </tr>
  </table>
</div>



### Power

There are a few power-related nets broken out to Qwiic connectors and through hole pads. The recommended input voltage is **3.3V**. The logic levels for the Qwiic Buzzer is **3.3V**.

* **3V3** &mdash; This connects to the 3.3V net. This net should only be provided with a clean 3.3V power signal. This is also connected to the Qwiic connectors.
* **GND** &mdash; Of course, is the common, ground voltage (0V reference) for the system.

<div style="text-align: center;">
  <table>
    <tr style="vertical-align:middle;">
     <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><a href="../assets/img/BOB-24474-Qwiic-Buzzer_Power_Top_View.jpg"><img src="../assets/img/BOB-24474-Qwiic-Buzzer_Power_Top_View.jpg" width="600px" height="600px" alt="Power Nets Highlighted, Top View"></a></td>
     <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><a href="../assets/img/BOB-24474-Qwiic-Buzzer_Power_Bottom_View.jpg"><img src="../assets/img/BOB-24474-Qwiic-Buzzer_Power_Bottom_View.jpg" width="600px" height="600px" alt="Power Nets Highlighted, Bottom View"></a></td>
    </tr>
    <tr style="vertical-align:middle;">
     <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><i>Power Nets Highlighted<br />(Top View)</i></td>
     <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><i>Power Nets Highlighted<br />(Bottom View)</i></td>
    </tr>
  </table>
</div>

!!! note
    While the ATtiny84 microcontroller can handle a voltage between 1.1V to 5.5V, we recommend only using 3.3V due to the fact that the Qwiic port's I<sup>2</sup>C data lines use are typically interfacing with a 3.3V system.



### Buzzer

!!! note
    Interested in the difference between a magnetic and piezo buzzer? Check out the article from CUI Devices: "[Buzzer Basics - Technologies, Tones, and Drive Circuits](https://www.cuidevices.com/blog/buzzer-basics-technologies-tones-and-driving-circuits#magnetic-and-piezo-buzzers)".

The board would not be the Qwiic Buzzer without... a buzzer! The buzzer uses a small magnetic coil to vibrate a metal disc inside the plastic housing. By pulsating current through the coil at different rates, we can produce different frequencies (pitches) of sound.

<div style="text-align: center;">
  <table>
    <tr style="vertical-align:middle;">
      <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><a href="../assets/img/BOB-24474-Qwiic-Buzzer_Buzzer_Transistor_Diode.jpg"><img src="../assets/img/BOB-24474-Qwiic-Buzzer_Buzzer_Transistor_Diode.jpg" width="600px" height="600px" alt="Buzzer Highlighted"></a></td>
    </tr>
    <tr style="vertical-align:middle;">
      <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><i>Buzzer Highlighted<br />(Top View)</i></td>
    </tr>
  </table>
</div>

Power and control is applied to the buzzer with a set of BJTs. At full volume, it allows the full current of about ~95mA, which makes it _really_ loud. Typically, power and control is directly connected to microcontrollers GPIO, which limits the current to ~40mA. A [flyback diode](https://learn.sparkfun.com/tutorials/diodes/all#diode-applications) is included to discharge any energy remaining when the buzzer is shut off.



### ATtiny84

The brains of the Qwiic Buzzer users an ATtiny84. This IC comes pre-programmed with custom firmware designed to interact with the Arduino Library. The ATtiny84 accepts I<sup>2</sup>C reads and writes, interprets them, and creates sound by sending a PWM signal to the buzzer. The volume of the buzzer is typically controlled using a variable resistor like a potentiometer. However, four GPIOs, NPN transistors, and resistors are used to provide 7 levels of loudness.

<div style="text-align: center;">
  <table>
    <tr style="vertical-align:middle;">
     <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><a href="../assets/img/BOB-24474-Qwiic-Buzzer_ATtiny85_Microcontroller.jpg"><img src="../assets/img/BOB-24474-Qwiic-Buzzer_ATtiny85_Microcontroller.jpg" width="600px" height="600px" alt="ATtiny84 microcontroller Highlighted"></a></td>
    </tr>
    <tr style="vertical-align:middle;">
      <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><i>ATTiny84 Highlighted<br />(Top View)</i></td>
    </tr>
  </table>
</div>

!!! note "Where's SPI?!?"
    This board was designed to control the buzzer via I<sup>2</sup>C. While you can access the SPI pins with the help of the Eagle schematic and board files (e.g. <b><span style="text-decoration: overline">TRIG</span></b> = POCI, SDA = PICO, and SCL = SCK), these pins are only used for advanced users and programing the AVR microcontroller using custom firmware. For more information about programming an ATtiny84, check out our [Tiny AVR Programmer Hookup Guide](https://learn.sparkfun.com/tutorials/tiny-avr-programmer-hookup-guide#introduction).



### Qwiic and I<sup>2</sup>C

There are two PTHs labeled **SDA** and **SCL** on one side of the board. These indicate the I<sup>2</sup>C data and clock lines and are connected to two 2.2k&ohm; pull-up resistors. We also conveniently added a GND and 3.3V pin on one side should you decide to daisy chain additional I<sup>2</sup>C devices to the PTH. Similarly, you can use either of the Qwiic connectors to provide power and send data through I<sup>2</sup>C. The [Qwiic ecosystem](https://www.sparkfun.com/qwiic) is made for fast prototyping by removing the need for soldering. All you need to do is plug a Qwiic cable into the Qwiic connector and voila!

* **SCL** &mdash; I<sup>2</sup>C clock
* **SDA** &mdash; I<sup>2</sup>C data

<div style="text-align: center;">
  <table>
    <tr style="vertical-align:middle;">
     <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><a href="../assets/img/BOB-24474-Qwiic-Buzzer_I2C_Top_View.jpg"><img src="../assets/img/BOB-24474-Qwiic-Buzzer_I2C_Top_View.jpg" width="600px" height="600px" alt="Qwiic Connectors and I2C Port Highlighted"></a></td>
     <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><a href="../assets/img/BOB-24474-Qwiic-Buzzer_I2C_Bottom_View.jpg"><img src="../assets/img/BOB-24474-Qwiic-Buzzer_I2C_Bottom_View.jpg" width="600px" height="600px" alt="Qwiic Connectors and I2C Port Highlighted"></a></td>
    </tr>
    <tr style="vertical-align:middle;">
     <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><i>I<sup>2</sup>C PTHs Highlighted<br />(Top View)</i></td>
     <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><i>Qwiic and I<sup>2</sup>C PTHs Highlighted<br />(Bottom View)</i></td>
    </tr>
  </table>
</div>

The default I<sup>2</sup>C address for the Qwiic buzzer is **0x34**. This can also be software configured.



### Broken Out Pins


* <b><span style="text-decoration: overline">TRIG</span></b> &mdash; The trigger pin enables users to active the buzzer without an I<sup>2</sup>C whenever the pin is pulled low. This pin also has an alternative function. Pulling the trigger pin low as the board is initially powered up will factory reset the board.
* <b><span style="text-decoration: overline">RST</span></b> &mdash; The reset pin resets the ATTiny84 when the pin is pulled low.

<div style="text-align: center;">
  <table>
    <tr style="vertical-align:middle;">
     <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><a href="../assets/img/BOB-24474-Qwiic-Buzzer_Interrupt_Reset_Top_View.jpg"><img src="../assets/img/BOB-24474-Qwiic-Buzzer_Interrupt_Reset_Top_View.jpg" width="600px" height="600px" alt="Reset PTH Highlighted"></a></td>
     <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><a href="../assets/img/BOB-24474-Qwiic-Buzzer_Interrupt_Reset_Bottom_View.jpg"><img src="../assets/img/BOB-24474-Qwiic-Buzzer_Interrupt_Reset_Bottom_View.jpg" width="600px" height="600px" alt="Reset PTH Highlighted"></a></td>
    </tr>
    <tr style="vertical-align:middle;">
     <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><i>Trigger & Reset PTHs Highlighted<br />(Top View)</i></td>
     <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><i>Trigger & Reset PTHs Highlighted<br />(Bottom View)</i></td>
    </tr>
  </table>
</div>



### User Resistor

There is a footprint available for a PTH resistor. This is labeled as USER. For users interested in including a custom resistor to control the loudness of the buzzer, you can solder a PTH resistor to the two PTHs. Note that you would need to cut the jumper on the back of the board (labeled as JP1) if you are not using the onboard 2.2k&ohm; SMD resistor.

<div style="text-align: center;">
  <table>
    <tr style="vertical-align:middle;">
     <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><a href="../assets/img/BOB-24474-Qwiic-Buzzer_User_Resistor_Top_Volume_View.jpg"><img src="../assets/img/BOB-24474-Qwiic-Buzzer_User_Resistor_Top_Volume_View.jpg" width="600px" height="600px" alt="USER PTH resistor footprint"></a></td>
     <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><a href="../assets/img/BOB-24474-Qwiic-Buzzer_User_Resistor_Bottom_Volume_View.jpg"><img src="../assets/img/BOB-24474-Qwiic-Buzzer_User_Resistor_Bottom_Volume_View.jpg" width="600px" height="600px" alt="USER PTH resistor footprint"></a></td>
    </tr>
    <tr style="vertical-align:middle;">
     <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><i>User Resistor PTHs Highlighted<br />(Top View)</i></td>
     <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><i>User Resistor PTHs Highlighted<br />(Bottom View)</i></td>
    </tr>
  </table>
</div>



### LEDs

The board includes the following status LEDs as indicated in the image below.

* **PWR** &mdash; The power LED lights up when the board is powered. This LED is connected the 3.3V net. This LED can be disabled with the PWR jumper on the bottom of the board.
* **STAT** &mdash; The status LED lights up whenever the buzzer is active. This adds a nice visual indicator whenever there is noise.

<div style="text-align: center;">
  <table>
    <tr style="vertical-align:middle;">
     <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><a href="../assets/img/BOB-24474-Qwiic-Buzzer_LEDs.jpg"><img src="../assets/img/BOB-24474-Qwiic-Buzzer_LEDs.jpg" width="600px" height="600px" alt="LEDs Highlighted"></a></td>
    </tr>
    <tr style="vertical-align:middle;">
     <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><i>LEDs Highlighted<br />(Top View)</i></td>
    </tr>
  </table>
</div>



### Jumpers

!!!note
    If this is your first time working with jumpers, check out the [How to Work with Jumper Pads and PCB Traces](https://learn.sparkfun.com/tutorials/how-to-work-with-jumper-pads-and-pcb-traces/all) tutorial for more information.

If you flip the board over, you will notice a few jumper pads.

* **PWR_LED** &mdash; The power LED will illuminate when 3.3V is available either over the Qwiic bus or the 3V3 pin. By default, this jumper is closed. Cut this jumper to disable the LED.
* **STAT** &mdash; The status LED will illuminate whenever the buzzer is making noise. By default, this jumper is closed. Cut this jumper to disable the LED.
* **I2C** &mdash; This three way jumper labeled **I2C** is connected to two pull-up resistors to the I<sup>2</sup>C data and clock lines. For users that have multiple Qwiic-enabled devices with pull-up resistors enabled, the parallel equivalent resistance will create too strong of a pull-up for the bus to operate correctly. As a general rule of thumb, [disable all but one pair of pull-up resistors](https://learn.sparkfun.com/tutorials/i2c/all#i2c-at-the-hardware-level) if multiple devices are connected to the bus.
* **JP1** &mdash; This jumper pad is available for users that want include a custom resistor to control the loudness of the buzzer. Cut this jumper to disconnect resistor R15. You will need to have a resistor populated for the USER PTH resistor.

<div style="text-align: center;">
  <table>
    <tr style="vertical-align:middle;">
     <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><a href="../assets/img/BOB-24474-Qwiic-Buzzer_Jumpers.jpg"><img src="../assets/img/BOB-24474-Qwiic-Buzzer_Jumpers.jpg" width="600px" height="600px" alt="Jumpers Highlighted"></a></td>
    </tr>
    <tr style="vertical-align:middle;">
     <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><i>Jumpers Highlighted<br />(Bottom View)</i></td>
    </tr>
  </table>
</div>




### Board Dimensions

The board uses the standard 1.0" x 1.0" (25.4mm x 25.4mm) Qwiic board. There are 4x mounting holes by each corner of the board.

<div style="text-align: center;">
  <table>
    <tr style="vertical-align:middle;">
     <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><a href="../assets/img/BOB-24474-Qwiic-Buzzer__Board_Dimensions.png"><img src="../assets/img/BOB-24474-Qwiic-Buzzer__Board_Dimensions.png" width="600px" height="600px" alt="Board Dimensions"></a></td>
    </tr>
    <tr style="vertical-align:middle;">
     <td style="text-align: center; border: solid 1px #cccccc;" vertical-align: middle;"><i>Board Dimensions</i></td>
    </tr>
  </table>
</div>
