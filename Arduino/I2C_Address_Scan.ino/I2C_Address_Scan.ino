/**
 * TCA9548 I2CScanner.ino -- I2C bus scanner for Arduino
 *
 * Based on https://playground.arduino.cc/Main/I2cScanner/
 *
 */

#include "Wire.h"
#include <EEPROM.h>
#include <avr/wdt.h>

#define TCAADDR 0x70
#define RESET_PIN 13

// EEPROM addresses
#define EEPROM_MAGIC_ADDR 0      // Magic number to detect valid data
#define EEPROM_FAILED_PORT_ADDR 1 // Which port failed
#define EEPROM_RETRY_COUNT_ADDR 2 // How many retries we've done
#define EEPROM_SKIP_PORTS_ADDR 3  // Bitmap of ports to skip

#define MAGIC_NUMBER 0xAB
#define MAX_RETRIES 3

struct FailureState {
  uint8_t magic;
  uint8_t failedPort;
  uint8_t retryCount;
  uint8_t skipPortsBitmap;  // Bit 0 = port 0, bit 1 = port 1, etc.
};

FailureState failureState;

void saveFailureState() {
  EEPROM.write(EEPROM_MAGIC_ADDR, failureState.magic);
  EEPROM.write(EEPROM_FAILED_PORT_ADDR, failureState.failedPort);
  EEPROM.write(EEPROM_RETRY_COUNT_ADDR, failureState.retryCount);
  EEPROM.write(EEPROM_SKIP_PORTS_ADDR, failureState.skipPortsBitmap);
}

void loadFailureState() {
  failureState.magic = EEPROM.read(EEPROM_MAGIC_ADDR);
  failureState.failedPort = EEPROM.read(EEPROM_FAILED_PORT_ADDR);
  failureState.retryCount = EEPROM.read(EEPROM_RETRY_COUNT_ADDR);
  failureState.skipPortsBitmap = EEPROM.read(EEPROM_SKIP_PORTS_ADDR);
  
  // If no valid data found, initialize
  if (failureState.magic != MAGIC_NUMBER) {
    failureState.magic = MAGIC_NUMBER;
    failureState.failedPort = 255; // No failed port
    failureState.retryCount = 0;
    failureState.skipPortsBitmap = 0;
    saveFailureState();
  }
}

void clearFailureState() {
  failureState.magic = MAGIC_NUMBER;
  failureState.failedPort = 255;
  failureState.retryCount = 0;
  failureState.skipPortsBitmap = 0;
  saveFailureState();
}

void markPortAsFailed(uint8_t port) {
  failureState.failedPort = port;
  failureState.retryCount++;
  
  // If we've retried too many times, permanently skip this port
  if (failureState.retryCount >= MAX_RETRIES) {
    failureState.skipPortsBitmap |= (1 << port);
    failureState.retryCount = 0;
    failureState.failedPort = 255;
    Serial.print("Port ");
    Serial.print(port);
    Serial.println(" permanently marked as failed after max retries");
  }
  
  saveFailureState();
}

bool shouldSkipPort(uint8_t port) {
  return (failureState.skipPortsBitmap & (1 << port)) != 0;
}

void resetArduino() {
  Serial.println("Resetting Arduino due to I2C bus failure...");
  Serial.flush();
  delay(100);
  
  // Enable watchdog timer with shortest timeout
  wdt_enable(WDTO_15MS);
  
  // Wait for watchdog to reset
  while(1);
}

void tcaReset() {
  // Perform initial reset sequence
  digitalWrite(RESET_PIN, LOW);
  delay(10);
  digitalWrite(RESET_PIN, HIGH);
  delay(50);
}

bool tcaselect(uint8_t i)
{
  if (i > 7)
    return false;

  // Check if this port should be skipped
  if (shouldSkipPort(i)) {
    Serial.print("Skipping port ");
    Serial.print(i);
    Serial.println(" (permanently failed)");
    return false;
  }

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

  delay(10);
  
  uint8_t result = Wire.endTransmission();
  
  // Check if timeout occurred
  if (Wire.getWireTimeoutFlag()) {
    Serial.println("I2C TIMEOUT occurred during TCA selection!");
    Wire.clearWireTimeoutFlag();
    
    // Mark this port as failed and reset
    markPortAsFailed(i);
    resetArduino();
    return false; // This won't be reached, but for completeness
  }
  
  Serial.print("TCA Return Code: ");
  Serial.println(result);
  Serial.flush();
  
  if (result != 0) {
    Serial.print("ERROR: TCA9548 not responding, code: ");
    Serial.println(result);
    Serial.flush();
    
    // Mark this port as failed and reset
    markPortAsFailed(i);
    resetArduino();
    return false; // This won't be reached, but for completeness
  }
  
  delay(50);
  return true;
}

void setup()
{
  // Disable watchdog timer first thing
  wdt_disable();
  
  Serial.begin(115200);
  while (!Serial);
  delay(1000);

  // Load failure state from EEPROM
  loadFailureState();
  
  // Print recovery info if we're recovering from a failure
  if (failureState.failedPort != 255) {
    Serial.print("Recovering from port ");
    Serial.print(failureState.failedPort);
    Serial.print(" failure (retry ");
    Serial.print(failureState.retryCount);
    Serial.print("/");
    Serial.print(MAX_RETRIES);
    Serial.println(")");
  }
  
  if (failureState.skipPortsBitmap != 0) {
    Serial.print("Permanently skipping ports: ");
    for (uint8_t i = 0; i < 8; i++) {
      if (shouldSkipPort(i)) {
        Serial.print(i);
        Serial.print(" ");
      }
    }
    Serial.println();
  }

  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);
  
  tcaReset();
  
  Wire.begin();
  Wire.setWireTimeout(1000000, true);
  
  Serial.println("\nTCAScanner ready!");
  
  Serial.println("Starting initial scan...");
  Serial.flush();
  scanAllPorts();
  
  // If we completed successfully, clear any previous failures
  // clearFailureState();
  // Serial.println("Scan completed successfully - cleared failure state");
}

void loop()
{
  delay(5000);
  if (Serial.available()) {
    while(Serial.available()) {
      char c = Serial.read();
      if (c == 'r' || c == 'R') {
        // Reset failure state on 'r' command
        clearFailureState();
        Serial.println("Failure state cleared!");
      }
    }
    Serial.println("\nRescanning...");
    Serial.flush();
    scanAllPorts();
  }
}

void scanAllPorts()
{
  Serial.println("Testing TCA9548 presence...");
  Serial.flush();
  
  Wire.beginTransmission(TCAADDR);
  uint8_t tcaTest = Wire.endTransmission();
  
  if (Wire.getWireTimeoutFlag()) {
    Serial.println("TIMEOUT: TCA9548 not responding");
    Wire.clearWireTimeoutFlag();
    Serial.flush();
    return;
  }
  
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
    
    bool tcaSuccess = tcaselect(t);
    
    if (!tcaSuccess) {
      Serial.print("Skipping device scan for port ");
      Serial.print(t);
      Serial.println(" due to TCA selection failure");
      Serial.flush();
      continue;
    }
    
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

      if (Wire.getWireTimeoutFlag()) {
        Serial.print("Timeout at address 0x");
        Serial.println(addr, HEX);
        Wire.clearWireTimeoutFlag();
        Serial.flush();
        continue;
      }

      if (status == 0)
      {
        Serial.print("Found I2C 0x");
        Serial.println(addr, HEX);
        Serial.flush();
        deviceCount++;
      }
      
      delay(5);
      
      if (addr % 32 == 0) {
        yield();
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
  Serial.println("\nScan complete. Send any character to rescan, 'r' to reset failure state.");
  Serial.flush();
}