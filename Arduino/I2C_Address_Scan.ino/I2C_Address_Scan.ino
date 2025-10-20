/**
 * TCA9548 I2CScanner.ino -- I2C bus scanner for Arduino
 *
 * Based on https://playground.arduino.cc/Main/I2cScanner/
 *
 */

#include "Wire.h"

#define TCAADDR 0x70
#define RESET_PIN 13

void tcaReset() {
  // Perform initial reset sequence
  digitalWrite(RESET_PIN, LOW);
  delay(10);
  digitalWrite(RESET_PIN, HIGH);
  delay(50);
}

void tcaselect(uint8_t i)
{
  if (i > 7)
    return;

  Serial.print("Selecting TCA port: ");
  Serial.println(i);
  Serial.flush();

  Wire.beginTransmission(TCAADDR);
  uint8_t bit = 1 << i;
  Serial.print("Mux Bit: ");
  Serial.println(bit, HEX);
  Serial.flush();
  
  Wire.write(bit);
  Serial.println("Write complete, ending transmission...");
  Serial.flush();

  // Add a small delay before endTransmission
  delay(10);
  
  uint8_t result = Wire.endTransmission();
  if (result) {
    Serial.println("TCA port selection failed.");
    Serial.print("Error code: ");
    Serial.println(result);
    Serial.flush();
  }
  
  if (result != 0) {
    Serial.print("ERROR: TCA9548 not responding, code: ");
    Serial.println(result);
    Serial.flush();
    
    // Try to recover the I2C bus
    Wire.end();
    delay(100);
    Wire.begin();
    delay(50);
    return;
  }
  
  delay(50);
}

// standard Arduino setup()
void setup()
{
  Serial.begin(115200);
  while (!Serial);
  delay(1000);

  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);
  
  // Perform initial reset
  tcaReset();
  
  Wire.begin();
  Serial.println("\nTCAScanner ready!");
  
  // Run initial scan
  Serial.println("Starting initial scan...");
  Serial.flush();
  scanAllPorts();
}

void loop()
{
  delay(5000);
  if (Serial.available()) {
    // Clear entire buffer
    while(Serial.available()) {
      Serial.read();
    }
    Serial.println("\nRescanning...");
    Serial.flush();
    scanAllPorts();
  }
}

void scanAllPorts()
{
  // First, test if TCA9548 is present
  Serial.println("Testing TCA9548 presence...");
  Serial.flush();
  
  Wire.beginTransmission(TCAADDR);
  uint8_t tcaTest = Wire.endTransmission();
  
  if (tcaTest != 0) {
    Serial.print("ERROR: TCA9548 not found at address 0x");
    Serial.print(TCAADDR, HEX);
    Serial.print(", error code: ");
    Serial.println(tcaTest);
    Serial.flush();
    return;
  }
  
  Serial.println("TCA9548 found, starting port scan...");
  Serial.flush();

  for (uint8_t t = 0; t < 8; t++)
  {
    Serial.print("About to select port ");
    Serial.println(t);
    Serial.flush();
    
    // Skip problematic port 4 for now
    if (t == 4) {
      Serial.println("Skipping port 4 (known issue)");
      Serial.flush();
      continue;
    }
    
    tcaselect(t);
    
    Serial.print("TCA Port #");
    Serial.println(t);
    Serial.flush();

    uint8_t deviceCount = 0;
    for (uint8_t addr = 0; addr <= 127; addr++)
    {
      if (addr == TCAADDR)
        continue;

      Wire.beginTransmission(addr);
      uint8_t status = Wire.endTransmission();

      if (status == 0)
      {
        Serial.print("Found I2C 0x");
        Serial.println(addr, HEX);
        Serial.flush();
        deviceCount++;
      }
      
      delay(5);
      
      // Add periodic yields to prevent watchdog timeouts
      if (addr % 32 == 0) {
        yield(); // Allow other processes to run
        delay(10);
      }
    }
    
    Serial.print("Mux Address: ");
    Serial.print(t);
    Serial.print(" Complete (");
    Serial.print(deviceCount);
    Serial.println(" devices found)");
    Serial.flush();
    
    delay(100);
  }
  Serial.println("\nScan complete. Send any character to rescan.");
  Serial.flush();
}