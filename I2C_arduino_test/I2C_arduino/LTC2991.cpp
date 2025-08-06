#include "LTC2991.h"

bool writeData(int addr, int command, int value)
{
  int error;
  Wire.beginTransmission(addr);
  Wire.write(command);
  Wire.write(value);
  error = Wire.endTransmission();
  if ( error ){
    Serial.print("Wire Error: ");
    Serial.println(error);
    Serial.print("Addr: ");
    Serial.println(addr);
    Serial.print("Register: ");
    Serial.println(command);
    Serial.print("Value: ");
    Serial.println(value);
    return(1);
    }
    else
    {
//      Serial.println("Write Success!");
      return(0);
    }
}

int readData(int addr, int command)
{
  int error;
  int data;
  Wire.beginTransmission(addr);
  Wire.write(command);
  error = Wire.endTransmission();
  if ( error ){
    Serial.print("Wire Error: ");
    Serial.println(error);
    Serial.print("Addr: ");
    Serial.println(addr);
    Serial.print("Register: ");
    Serial.println(command);

  }
  int availBytes = Wire.requestFrom(addr,1);
  if(availBytes)
  {
      if(Wire.available())
      {
              

        data=Wire.read();
//        Serial.print("DEBUG-readData[");
//        Serial.print(i);
//        Serial.print("] = ");
//        Serial.println(data[i]);
      }
      else {
        Serial.println("Error, no data avalaible to read");
        
      }
    
    return(data);
  } 
  else
  {
      Serial.println("No data returned from read");
      return (0);
  }
  
  
}



float convertADCCurrent(int data)
{
  float voltage;
  int16_t sign =1;
  int code = data & 0x7FFF;  
  if (code >>14 )
  {
    code = ( code ^ 0x7FFF)+1;
    sign =-1;
    
  }
  voltage = ((float)code)*sign*LTC2991_DIFFERENTIAL_lsb;
  float  current = voltage/0.1; //resistance is hardcoded to 0.1 ohms
  return (current);
}

bool init(int dev[], int len)
{
  bool error;
  for(int i=0; i<len; i++)
  {
    Serial.print("Init: ");
    Serial.println(dev[i]);
    if( writeData(dev[i],0x01,0xF8)) error = true; 
    if( writeData(dev[i],0x06,0x00)) error = true; 
    if( writeData(dev[i],0x07,0x00)) error = true; 
    if( writeData(dev[i],0x08,0x10)) error = true; 
    
    // configure all channels on 
    if( writeData(dev[i],0x06,0x11)) error = true; 
    if( writeData(dev[i],0x07,0x11)) error = true; 
  }
  return (error);
}


int readCurrent(int addr, int regs, float &current )
{
    int data;
    
    readTimeout(addr,regs, data);
    delay(100);
    readTimeout(addr,regs, data);
    bool dataValid;
    dataValid = (data >> 15)& 0x01;
    if(dataValid){
      current = convertADCCurrent(data);
      return (0) ;
    }
    else Serial.println("Data not Valid");
    
}

int readTimeout(int addr, int regs, int &mydata)
{
    union
  {
    uint8_t b[2];
    uint16_t w;
  } data;
  int status;

  for (int timer=0; timer < 1000; timer++)
  {
      status = readData(addr,0x01);
      if (status == 0xFF) break;
      else if (timer == 999)
      {
        Serial.println("Timeout waiting for status");
        return(1);
      }
      delay(1);
  }
  
    //read to get rid of stale data
    data.b[1] = readData(addr,regs);
    data.b[0] = readData(addr,regs+1);
    mydata = data.w;
    return (0);
}
