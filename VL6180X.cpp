#include <VL6180X.h>
#include <Wire.h>

// Defines /////////////////////////////////////////////////////////////////////

// The Arduino two-wire interface uses a 7-bit number for the address,
// and sets the last bit correctly based on reads and writes
#define ADDRESS_DEFAULT 0b0101001

// RANGE_SCALER values for 1x, 2x, 3x scaling - see STSW-IMG003 core/src/vl6180x_api.c (ScalerLookUP[])
static uint16_t const ScalerValues[] = {0, 253, 127, 84};

// Constructors ////////////////////////////////////////////////////////////////

VL6180X::VL6180X()
  : bus(&Wire)
  , address(ADDRESS_DEFAULT)
  , scaling(0)
  , ptp_offset(0)
  , io_timeout(0) // no timeout
  , did_timeout(false)
{
}

// Public Methods //////////////////////////////////////////////////////////////

void VL6180X::setAddress(uint8_t new_addr)
{
  writeReg(I2C_SLAVE__DEVICE_ADDRESS, new_addr & 0x7F);
  address = new_addr;
}

// Initialize sensor with settings from ST application note AN4545, section
// "SR03 settings" - "Mandatory : private registers"
void VL6180X::init()
{
  // Store part-to-part range offset so it can be adjusted if scaling is changed
  ptp_offset = readReg(SYSRANGE__PART_TO_PART_RANGE_OFFSET);

  if (readReg(SYSTEM__FRESH_OUT_OF_RESET) == 1)
  {
    scaling = 1;

    writeReg(0x207, 0x01);
    writeReg(0x208, 0x01);
    writeReg(0x096, 0x00);
    writeReg(0x097, 0xFD); // RANGE_SCALER = 253
    writeReg(0x0E3, 0x01);
    writeReg(0x0E4, 0x03);
    writeReg(0x0E5, 0x02);
    writeReg(0x0E6, 0x01);
    writeReg(0x0E7, 0x03);
    writeReg(0x0F5, 0x02);
    writeReg(0x0D9, 0x05);
    writeReg(0x0DB, 0xCE);
    writeReg(0x0DC, 0x03);
    writeReg(0x0DD, 0xF8);
    writeReg(0x09F, 0x00);
    writeReg(0x0A3, 0x3C);
    writeReg(0x0B7, 0x00);
    writeReg(0x0BB, 0x3C);
    writeReg(0x0B2, 0x09);
    writeReg(0x0CA, 0x09);
    writeReg(0x198, 0x01);
    writeReg(0x1B0, 0x17);
    writeReg(0x1AD, 0x00);
    writeReg(0x0FF, 0x05);
    writeReg(0x100, 0x05);
    writeReg(0x199, 0x05);
    writeReg(0x1A6, 0x1B);
    writeReg(0x1AC, 0x3E);
    writeReg(0x1A7, 0x1F);
    writeReg(0x030, 0x00);

    writeReg(SYSTEM__FRESH_OUT_OF_RESET, 0);
  }
  else
  {
    // Sensor has already been initialized, so try to get scaling settings by
    // reading registers.

    uint16_t s = readReg16Bit(RANGE_SCALER);

    if      (s == ScalerValues[3]) { scaling = 3; }
    else if (s == ScalerValues[2]) { scaling = 2; }
    else                           { scaling = 1; }

    // Adjust the part-to-part range offset value read earlier to account for
    // existing scaling. If the sensor was already in 2x or 3x scaling mode,
    // precision will be lost calculating the original (1x) offset, but this can
    // be resolved by resetting the sensor and Arduino again.
    ptp_offset *= scaling;
  }
}

// Configure some settings for the sensor's default behavior from AN4545 -
// "Recommended : Public registers" and "Optional: Public registers"
//
// Note that this function does not set up GPIO1 as an interrupt output as
// suggested, though you can do so by calling:
// writeReg(SYSTEM__MODE_GPIO1, 0x10);
void VL6180X::configureDefault()
{
  // "Recommended : Public registers"

  // readout__averaging_sample_period = 48
  writeReg(READOUT__AVERAGING_SAMPLE_PERIOD, 0x30);

  // sysals__analogue_gain_light = 6 (ALS gain = 1 nominal, actually 1.01 according to table "Actual gain values" in datasheet)
  writeReg(SYSALS__ANALOGUE_GAIN, 0x46);

  // sysrange__vhv_repeat_rate = 255 (auto Very High Voltage temperature recalibration after every 255 range measurements)
  writeReg(SYSRANGE__VHV_REPEAT_RATE, 0xFF);

  // sysals__integration_period = 99 (100 ms)
  writeReg16Bit(SYSALS__INTEGRATION_PERIOD, 0x0063);

  // sysrange__vhv_recalibrate = 1 (manually trigger a VHV recalibration)
  writeReg(SYSRANGE__VHV_RECALIBRATE, 0x01);


  // "Optional: Public registers"

  // sysrange__intermeasurement_period = 9 (100 ms)
  writeReg(SYSRANGE__INTERMEASUREMENT_PERIOD, 0x09);

  // sysals__intermeasurement_period = 49 (500 ms)
  writeReg(SYSALS__INTERMEASUREMENT_PERIOD, 0x31);

  // als_int_mode = 4 (ALS new sample ready interrupt); range_int_mode = 4 (range new sample ready interrupt)
  writeReg(SYSTEM__INTERRUPT_CONFIG_GPIO, 0x24);


  // Reset other settings to power-on defaults

  // sysrange__max_convergence_time = 49 (49 ms)
  writeReg(VL6180X::SYSRANGE__MAX_CONVERGENCE_TIME, 0x31);

  // disable interleaved mode
  writeReg(INTERLEAVED_MODE__ENABLE, 0);

  // reset range scaling factor to 1x
  setScaling(1);
}

// Writes an 8-bit register
void VL6180X::writeReg(uint16_t reg, uint8_t value)
{
  bus->beginTransmission(address);
  bus->write((uint8_t)(reg >> 8)); // reg high byte
  bus->write((uint8_t)(reg >> 0)); // reg low byte
  bus->write(value);
  last_status = bus->endTransmission();
}

// Writes a 16-bit register
void VL6180X::writeReg16Bit(uint16_t reg, uint16_t value)
{
  bus->beginTransmission(address);
  bus->write((uint8_t)(reg >> 8)); // reg high byte
  bus->write((uint8_t)(reg >> 0)); // reg low byte
  bus->write((uint8_t)(value >> 8)); // value high byte
  bus->write((uint8_t)(value >> 0)); // value low byte
  last_status = bus->endTransmission();
}

// Writes a 32-bit register
void VL6180X::writeReg32Bit(uint16_t reg, uint32_t value)
{
  bus->beginTransmission(address);
  bus->write((uint8_t)(reg >> 8)); // reg high byte
  bus->write((uint8_t)(reg >> 0)); // reg low byte
  bus->write((uint8_t)(value >> 24)); // value highest byte
  bus->write((uint8_t)(value >> 16));
  bus->write((uint8_t)(value >>  8));
  bus->write((uint8_t)(value >>  0)); // value lowest byte
  last_status = bus->endTransmission();
}

// Reads an 8-bit register
uint8_t VL6180X::readReg(uint16_t reg)
{
  uint8_t value;

  bus->beginTransmission(address);
  bus->write((uint8_t)(reg >> 8)); // reg high byte
  bus->write((uint8_t)(reg >> 0)); // reg low byte
  last_status = bus->endTransmission();

  bus->requestFrom(address, (uint8_t)1);
  value = bus->read();

  return value;
}

// Reads a 16-bit register
uint16_t VL6180X::readReg16Bit(uint16_t reg)
{
  uint16_t value;

  bus->beginTransmission(address);
  bus->write((uint8_t)(reg >> 8)); // reg high byte
  bus->write((uint8_t)(reg >> 0)); // reg low byte
  last_status = bus->endTransmission();

  bus->requestFrom(address, (uint8_t)2);
  value  = (uint16_t)bus->read() << 8; // value high byte
  value |=           bus->read();      // value low byte

  return value;
}

// Reads a 32-bit register
uint32_t VL6180X::readReg32Bit(uint16_t reg)
{
  uint32_t value;

  bus->beginTransmission(address);
  bus->write((uint8_t)(reg >> 8)); // reg high byte
  bus->write((uint8_t)(reg >> 0)); // reg low byte
  last_status = bus->endTransmission();

  bus->requestFrom(address, (uint8_t)4);
  value  = (uint32_t)bus->read() << 24; // value highest byte
  value |= (uint32_t)bus->read() << 16;
  value |= (uint16_t)bus->read() << 8;
  value |=           bus->read();       // value lowest byte

  return value;
}

// Set range scaling factor. The sensor uses 1x scaling by default, giving range
// measurements in units of mm. Increasing the scaling to 2x or 3x makes it give
// raw values in units of 2 mm or 3 mm instead. In other words, a bigger scaling
// factor increases the sensor's potential maximum range but reduces its
// resolution.

// Implemented using ST's VL6180X API as a reference (STSW-IMG003); see
// VL6180x_UpscaleSetScaling() in vl6180x_api.c.
void VL6180X::setScaling(uint8_t new_scaling)
{
  uint8_t const DefaultCrosstalkValidHeight = 20; // default value of SYSRANGE__CROSSTALK_VALID_HEIGHT

  // do nothing if scaling value is invalid
  if (new_scaling < 1 || new_scaling > 3) { return; }

  scaling = new_scaling;
  writeReg16Bit(RANGE_SCALER, ScalerValues[scaling]);

  // apply scaling on part-to-part offset
  writeReg(VL6180X::SYSRANGE__PART_TO_PART_RANGE_OFFSET, ptp_offset / scaling);

  // apply scaling on CrossTalkValidHeight
  writeReg(VL6180X::SYSRANGE__CROSSTALK_VALID_HEIGHT, DefaultCrosstalkValidHeight / scaling);

  // This function does not apply scaling to RANGE_IGNORE_VALID_HEIGHT.

  // enable early convergence estimate only at 1x scaling
  uint8_t rce = readReg(VL6180X::SYSRANGE__RANGE_CHECK_ENABLES);
  writeReg(VL6180X::SYSRANGE__RANGE_CHECK_ENABLES, (rce & 0xFE) | (scaling == 1));
}

// Performs a single-shot ranging measurement
uint8_t VL6180X::readRangeSingle()
{
  writeReg(SYSRANGE__START, 0x01);
  return readRangeContinuous();
}

// Performs a single-shot ambient light measurement
uint16_t VL6180X::readAmbientSingle()
{
  writeReg(SYSALS__START, 0x01);
  return readAmbientContinuous();
}

// Starts continuous ranging measurements with the given period in ms
// (10 ms resolution; defaults to 100 ms if not specified).
//
// The period must be greater than the time it takes to perform a
// measurement. See section "Continuous mode limits" in the datasheet
// for details.
void VL6180X::startRangeContinuous(uint16_t period)
{
  int16_t period_reg = (int16_t)(period / 10) - 1;
  period_reg = constrain(period_reg, 0, 254);

  writeReg(SYSRANGE__INTERMEASUREMENT_PERIOD, period_reg);
  writeReg(SYSRANGE__START, 0x03);
}

// Starts continuous ambient light measurements with the given period in ms
// (10 ms resolution; defaults to 500 ms if not specified).
//
// The period must be greater than the time it takes to perform a
// measurement. See section "Continuous mode limits" in the datasheet
// for details.
void VL6180X::startAmbientContinuous(uint16_t period)
{
  int16_t period_reg = (int16_t)(period / 10) - 1;
  period_reg = constrain(period_reg, 0, 254);

  writeReg(SYSALS__INTERMEASUREMENT_PERIOD, period_reg);
  writeReg(SYSALS__START, 0x03);
}

// Starts continuous interleaved measurements with the given period in ms
// (10 ms resolution; defaults to 500 ms if not specified). In this mode, each
// ambient light measurement is immediately followed by a range measurement.
//
// The datasheet recommends using this mode instead of running "range and ALS
// continuous modes simultaneously (i.e. asynchronously)".
//
// The period must be greater than the time it takes to perform both
// measurements. See section "Continuous mode limits" in the datasheet
// for details.
void VL6180X::startInterleavedContinuous(uint16_t period)
{
  int16_t period_reg = (int16_t)(period / 10) - 1;
  period_reg = constrain(period_reg, 0, 254);

  writeReg(INTERLEAVED_MODE__ENABLE, 1);
  writeReg(SYSALS__INTERMEASUREMENT_PERIOD, period_reg);
  writeReg(SYSALS__START, 0x03);
}

// Stops continuous mode. This will actually start a single measurement of range
// and/or ambient light if continuous mode is not active, so it's a good idea to
// wait a few hundred ms after calling this function to let that complete
// before starting continuous mode again or taking a reading.
void VL6180X::stopContinuous()
{

  writeReg(SYSRANGE__START, 0x01);
  writeReg(SYSALS__START, 0x01);

  writeReg(INTERLEAVED_MODE__ENABLE, 0);
}

// Returns a range reading when continuous mode is activated
// (readRangeSingle() also calls this function after starting a single-shot
// range measurement)
uint8_t VL6180X::readRangeContinuous()
{
  uint16_t millis_start = millis();
  while ((readReg(RESULT__INTERRUPT_STATUS_GPIO) & 0x04) == 0)
  {
    if (io_timeout > 0 && ((uint16_t)millis() - millis_start) > io_timeout)
    {
      did_timeout = true;
      return 255;
    }
  }

  uint8_t range = readReg(RESULT__RANGE_VAL);
  writeReg(SYSTEM__INTERRUPT_CLEAR, 0x01);

  return range;
}

// Returns an ambient light reading when continuous mode is activated
// (readAmbientSingle() also calls this function after starting a single-shot
// ambient light measurement)
uint16_t VL6180X::readAmbientContinuous()
{
  uint16_t millis_start = millis();
  while ((readReg(RESULT__INTERRUPT_STATUS_GPIO) & 0x20) == 0)
  {
    if (io_timeout > 0 && ((uint16_t)millis() - millis_start) > io_timeout)
    {
      did_timeout = true;
      return 0;
    }
  }

  uint16_t ambient = readReg16Bit(RESULT__ALS_VAL);
  writeReg(SYSTEM__INTERRUPT_CLEAR, 0x02);

  return ambient;
}

// Did a timeout occur in one of the read functions since the last call to
// timeoutOccurred()?
bool VL6180X::timeoutOccurred()
{
  bool tmp = did_timeout;
  did_timeout = false;
  return tmp;
}
