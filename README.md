# VL6180X library for Arduino

Version: 1.3.1<br>
Release date: 2021 Jun 29<br>
[www.pololu.com](https://www.pololu.com/)

## Summary

This is a library for the Arduino IDE that helps interface with ST's [VL6180X time-of-flight distance and ambient light sensor](https://www.pololu.com/product/2489). The library makes it simple to configure the sensor and read range and ambient light level data from it via I&sup2;C.

## Supported platforms

This library is designed to work with the Arduino IDE versions 1.6.x or later; we have not tested it with earlier versions.  This library should support any Arduino-compatible board, including the [Pololu A-Star 32U4 controllers](https://www.pololu.com/category/149/a-star-programmable-controllers).

## Getting started

### Hardware

A [VL6180X carrier](https://www.pololu.com/product/2489) can be purchased from Pololu's website.  Before continuing, careful reading of the [product page](https://www.pololu.com/product/2489) as well as the VL6180X datasheet and application notes is recommended.

Make the following connections between the Arduino and the VL6180X board:

#### 5V Arduino boards

(including Arduino Uno, Leonardo, Mega; Pololu A-Star 32U4)

    Arduino   VL6180X board
    -------   -------------
         5V - VIN
        GND - GND
        SDA - SDA
        SCL - SCL

#### 3.3V Arduino boards

(including Arduino Due)

    Arduino   VL6180X board
    -------   -------------
        3V3 - VIN
        GND - GND
        SDA - SDA
        SCL - SCL

### Software

If you are using version 1.6.2 or later of the [Arduino software (IDE)](http://www.arduino.cc/en/Main/Software), you can use the Library Manager to install this library:

1. In the Arduino IDE, open the "Sketch" menu, select "Include Library", then "Manage Libraries...".
2. Search for "VL6180X".
3. Click the VL6180X entry in the list.
4. Click "Install".

If this does not work, you can manually install the library:

1. Download the [latest release archive from GitHub](https://github.com/pololu/vl6180x-arduino/releases) and decompress it.
2. Rename the folder "vl6180x-arduino-master" to "VL6180X".
3. Move the "VL6180X" folder into the "libraries" directory inside your Arduino sketchbook directory.  You can view your sketchbook location by opening the "File" menu and selecting "Preferences" in the Arduino IDE.  If there is not already a "libraries" folder in that location, you should make the folder yourself.
4. After installing the library, restart the Arduino IDE.

## Examples

Several example sketches are available that show how to use the library. You can access them from the Arduino IDE by opening the "File" menu, selecting "Examples", and then selecting "VL6180X". If you cannot find these examples, the library was probably installed incorrectly and you should retry the installation instructions above.

## Library reference

* `uint8_t last_status`<br>
  The status of the last I&sup2;C write transmission. See the [`Wire.endTransmission()` documentation](http://arduino.cc/en/Reference/WireEndTransmission) for return values.

* `VL6180X(void)`<br>
  Constructor.

* `void setBus(TwoWire * bus)`<br>
  Configures this object to use the specified I&sup2;C bus. `bus` should be a pointer to a `TwoWire` object; the default bus is `Wire`, which is typically the first or only I&sup2;C bus on an Arduino. If your Arduino has more than one I&sup2;C bus and you have the VL6180X connected to the second bus, which is typically called `Wire1`, you can call `sensor.setBus(&Wire1);`.

* `TwoWire * getBus()`<br>
  Returns a pointer to the I&sup2;C bus this object is using.

* `void setAddress(uint8_t new_addr)`<br>
  Changes the I&sup2;C slave device address of the VL6180X to the given value (7-bit).

* `uint8_t getAddress()`<br>
  Returns the current I&sup2;C address.

* `void init()`<br>
  Loads required settings onto the VL6180X to initialize the sensor.

* `void configureDefault(void)`<br>
  Configures some settings for the sensor's default behavior. See the comments in VL6180X.cpp for a full explanation of the settings.

* `void writeReg(uint16_t reg, uint8_t value)`<br>
  Writes an 8-bit sensor register with the given value.

  Register address constants are defined by the regAddr enumeration type in VL6180X.h.<br>
  Example use: `sensor.writeReg(VL6180X::SYSRANGE__MAX_CONVERGENCE_TIME, 30);`

* `void writeReg16Bit(uint16_t reg, uint16_t value)`<br>
  Writes a 16-bit sensor register with the given value.

* `void writeReg32Bit(uint16_t reg, uint32_t value)`<br>
  Writes a 32-bit sensor register with the given value.

* `uint8_t readReg(uint16_t reg)`<br>
  Reads an 8-bit sensor register and returns the value read.

* `uint16_t readReg16Bit(uint16_t reg)`<br>
  Reads a 16-bit sensor register and returns the value read.

* `uint32_t readReg32Bit(uint16_t reg)`<br>
  Reads a 32-bit sensor register and returns the value read.

* `void setScaling(uint8_t new_scaling)`<br>
  Sets range scaling factor. The sensor uses 1x scaling by default, giving range measurements in units of mm. Increasing the scaling to 2x or 3x makes it give raw values in units of 2 mm or 3 mm instead. In other words, a bigger scaling factor increases the sensor's potential maximum range but reduces its resolution.

* `uint8_t getScaling(void)`<br>
  Returns the current range scaling factor.

* `uint8_t readRangeSingle(void)`<br>
  Performs a single-shot ranging measurement and returns the raw reading.

* `uint16_t readRangeSingleMillimeters(void)`<br>
  Performs a single-shot ranging measurement and returns the reading in millimeters, taking the range scaling setting into account.

* `uint16_t readAmbientSingle(void)`<br>
  Performs a single-shot ambient light measurement and returns the reading.

* `void startRangeContinuous(uint16_t period)`<br>
  Starts continuous ranging measurements with the given period in milliseconds (10 ms resolution; defaults to 100 ms if not specified).

  In all continuous modes, the period must be greater than the time it takes to perform the measurement(s). See section "Continuous mode limits" in the datasheet for details.

* `void startAmbientContinuous(uint16_t period)`<br>
  Starts continuous ambient light measurements with the given period in milliseconds (10 ms resolution; defaults to 500 ms if not specified).

* `void startInterleavedContinuous(uint16_t period)`<br>
   Starts continuous interleaved measurements with the given period in milliseconds (10 ms resolution; defaults to 500 ms if not specified).

   In this mode, each ambient light measurement is immediately followed by a range measurement. You should use this mode instead of enabling continuous mode for ranging and ambient light independently.

* `void stopContinuous(void)`<br>
  Stops continuous mode.

* `uint8_t readRangeContinuous(void)`<br>
  Returns a raw range reading when continuous mode is active.

* `uint16_t readRangeContinuousMillimeters(void)`<br>
  Returns a range reading in millimeters, taking the range scaling setting into account, when continuous mode is active.

* `uint16_t readAmbientContinuous(void)`<br>
  Returns an ambient light reading when continuous mode is active.

* `void setTimeout(uint16_t timeout)`<br>
  Sets a timeout period in milliseconds after which the read functions will abort if the sensor is not ready. A value of 0 disables the timeout.

* `uint16_t getTimeout(void)`<br>
  Returns the current timeout period setting.

* `bool timeoutOccurred(void)`<br>
  Indicates whether a read timeout has occurred since the last call to `timeoutOccurred()`.

## Version history
* 1.3.1 (2021 Jun 29): Fixed compilation errors with Arduino mbed core.
* 1.3.0 (2021 Jan 12): Added support for alternative I²C buses (thanks mjs513) and `getAddress()`. Fixed some minor code and documentation issues.
* 1.2.0 (2016 May 18): Added functions for reading range in millimeters, taking range scaling factor into account. Changed example sketches to use these functions.
* 1.1.0 (2016 May 12): Added functions to set range scaling factor and example sketch to demonstrate scaling.
* 1.0.1 (2016 Mar 14): Added missing `Serial.begin()` to examples and changed `configureDefault()` to reset some additional registers to power-on defaults.
* 1.0.0 (2015 Sep 24): Original release.
