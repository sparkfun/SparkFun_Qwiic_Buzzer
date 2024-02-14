SparkFun Qwiic Buzzer
========================================

[![SparkFun Qwiic Buzzer](https://cdn.sparkfun.com/r/600-600/assets/parts/2/4/8/3/2/BOB-24474-Qwiic-Buzzer-Feature.jpg)](https://www.sparkfun.com/products/24474)

[*SparkFun Qwiic Buzzer (BOB-24474)*](https://www.sparkfun.com/products/24474)

The SparkFun Qwiic Buzzer adds simple beeps and buzzes to your projects via I<sup>2</sup>C Make some noises to alert you when something interesting (or urgent) happens. Simply connect a Qwiic cable and load up some pre-written code to an Arduino to start making noises!

The board includes an ATtiny84 with custom firmware to control the piezoelectric buzzer over I<sup>2</sup>C. Utilizing our handy Qwiic system, no soldering is required to connect it to the rest of your system. However, we still broke out 0.1"-spaced PTH pads if you prefer to use a breadboard to access the board's I2C, power, trigger, and reset pins on the board's edge. A 2.2k&ohm; pull-up resistor is included on the data and clock lines. 

A set of transistors control the buzzer's volume. At full volume, it allows the full current of about ~95mA, which makes it _really_ loud. A flyback diode is included to discharge any energy remaining when the buzzer is turned off. For those who want to control the loudness of the buzzer, a footprint is available to solder a custom PTH resistor to one of the transistors. Just make sure to cut the jumper labeled "JP1." Two LEDs are also included on the board for power and status. 

We've written an Arduino Library to control the volume, frequency, duration, and I<sup>2</sup>C address. There are even examples of how to play a quick melody and sweet sound effects! You can download the library through the Arduino library manager by searching 'SparkFun Qwiic Buzzer,' or you can get the GitHub repo as a .zip to install the library from there manually. For those who want a simple buzz without an Arduino, you can also make noise by simply grounding the trigger pin! We've even ported the library as a Python package! 

This board is great for adding a simple audible sound to your projects. Pair the board with the Qwiic RTC to make an alarm clock, play a tune with the Qwiic RFID, or add a sound effect to your robot. So what are you waiting for?!? Start making some noise in your projects with the SparkFun Qwiic Buzzer!


Repository Contents
-------------------

* **.github/workflows** - YAML files used for GitHub Actions and GitHub Pages/mkdocs
* **/Firmware** - ATtiny84 Firmware
* **/Hardware** - Eagle design files (.brd, .sch)
  * **/Production** - Production panel files (.brd)
* **/docs** - Online documentation files
  * **/assets** - Folder containing all the file assets used for product documentation
    * **/board_files** - Copy of design files used for product documentation
    * **/component_documentation** - Datasheets and manuals for hardware components
    * **/img** - Images for product documentation
  * **/github** - Files stating how to contribute and filing issues used for product documentation
  * **/javascript** - Folder containing custom javascript used for product documentation
  * **/stylesheet** - Folder containing CSS files used for product documentation
* **/overrides** - Customization files used for product documentation
  * **/.icons** - Icons used for GitHub used for product documentation
  * **./partials** - Used for product documentation



Documentation
--------------

* **[Installing an Arduino Library Guide](https://learn.sparkfun.com/tutorials/installing-an-arduino-library)** - Basic information on how to install an Arduino library.
* **[Library](https://github.com/sparkfun/SparkFun_Qwiic_Buzzer_Arduino_Library)** - Arduino library for the Qwiic Buzzer.
* **[Hookup Guide](https://docs.sparkfun.com/SparkFun_Qwiic_Buzzer)** - Basic hookup guide for the  Qwiic Buzzer.



Product Versions
----------------
* [BOB-24474](https://www.sparkfun.com/products/24474) - v1.0



Version History
---------------
* v1.0 - Initial release



License Information
-------------------

This product is _**open source**_! 

Please review the LICENSE.md file for license information. 

If you have any questions or concerns on licensing, please contact technical support on our [SparkFun forums](https://forum.sparkfun.com/viewforum.php?f=152).

Distributed as-is; no warranty is given.

- Your friends at SparkFun.

_<COLLABORATION CREDIT>_
