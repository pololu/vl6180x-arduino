#include <Wire.h>
#include <VL6180X.h>

VL6180X sensor;

void setup() 
{
  Wire.begin();
  
  sensor.init();
  sensor.configureDefault();

  sensor.writeReg(VL6180X::SYSRANGE__MAX_CONVERGENCE_TIME, 30);
  sensor.writeReg16Bit(VL6180X::SYSALS__INTEGRATION_PERIOD, 50);
  
  sensor.setTimeout(500);

  sensor.stopContinuous();
  delay(300);
  sensor.startInterleavedContinuous(100);

}

void loop() 
{
  Serial.print("Ambient: ");
  Serial.print(sensor.readAmbientContinuous());
  if (sensor.timeoutOccurred()) { Serial.print(" TIMEOUT"); }
  
  Serial.print("\tRange: ");
  Serial.print(sensor.readRangeContinuous());
  if (sensor.timeoutOccurred()) { Serial.print(" TIMEOUT"); }

  Serial.println();
}
