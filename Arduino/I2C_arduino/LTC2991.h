#ifndef LTC2991_H
#define LTC2991_H

#include <Arduino.h>
#include <Wire.h>

// Constants
const float LTC2991_DIFFERENTIAL_lsb = 1.90735E-05;

// Function declarations
bool writeData(int addr, int command, int value);
int readData(int addr, int command);
float convertADCCurrent(int data);
bool init(int dev[], int len);
int readCurrent(int addr, int regs, float &current);
int readTimeout(int addr, int regs, int &mydata);

#endif // LTC2991_H
