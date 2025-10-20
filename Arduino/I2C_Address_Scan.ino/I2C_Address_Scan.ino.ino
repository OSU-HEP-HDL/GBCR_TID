/**
 * TCA9548 I2CScanner.ino -- I2C bus scanner for Arduino
 *
 * Based on https://playground.arduino.cc/Main/I2cScanner/
 *
 */

#include "Wire.h"

#define TCAADDR 0x70
int status;

void tcaselect(uint8_t i) {
  if (i > 7) return;

  digitalWrite(13, LOW);
  delay(100);
  digitalWrite(13, HIGH);
 
  Wire.beginTransmission(TCAADDR);
  int bit =1 <<i;
  Serial.print("Mux Bit: ");
  Serial.println(bit,HEX);
  Wire.write(bit);
  Serial.print("TCA Return Code: ");
  Serial.println(Wire.endTransmission()); 
  delay(1000); 
}


// standard Arduino setup()
void setup()
{
    while (!Serial);
    delay(1000);

    Wire.begin();
    
    Serial.begin(115200);
    Serial.println("\nTCAScanner ready!");
    
}

void loop() 
{
    
    for (uint8_t t=0; t<8; t++) {
      tcaselect(t);
      Serial.print("TCA Port #"); Serial.println(t);

      for (uint8_t addr = 0; addr<=127; addr++) {
        if (addr == TCAADDR) continue;

        Wire.beginTransmission(addr);
        status = Wire.endTransmission();
        
        if(!status) {
          Serial.print("Found I2C 0x");  Serial.println(addr,HEX);
        }
        else
        {
//          Serial.print("I2C Error ");
//          Serial.print(status);
//          Serial.print(" on address: ");
//          Serial.println(addr);
        }
      }
      Serial.print("Mux Address: ");
      Serial.print(t);
      Serial.println(" Complete");
    }
    Serial.println("\ndone");
}
