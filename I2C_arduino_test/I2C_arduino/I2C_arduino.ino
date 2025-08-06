#include <Wire.h>
#include "LTC2991.h"

bool debug = true;

byte regs[32] = {8, 187, 187, 117, 8, 187, 187, 117, 8, 187, 187, 117, 8, 187, 187, 117, 8, 187, 187, 117, 8, 187, 187, 117, 23, 249, 100, 23, 249, 100, 33, 66};

void setup() { 
  // Initialize Serial communication
  Serial.begin(115200); 
  Serial.setTimeout(250);
//  Initialize I2C communication
  Wire.begin();
  Wire.setClock(100000);
  Wire.setWireTimeout(3000, 1);
  // Set pin 13 as output for mux reset
  pinMode(13, OUTPUT);
}



void splitCom(String com, int *commands){
  int subIndex = 0;
  int index = 0;
  int rLength = com.length();

  while( subIndex < rLength ){
    String thisCode = com.substring(subIndex, subIndex+4);
    subIndex = subIndex + 4;

    commands[index] = thisCode.toInt();
    index = index+1;
  }
}

void runCom(String com){
  int commands[5];
  splitCom(com, commands);

  int w[3] = {-1, -1, -1};
  int muxOutput = -1;
  int regOutput = -1;

  switch(commands[0]){
    case 1111: //Turn on Power Supply
    acom_sendCom("PSU Address: " + String(commands[1],HEX));
      int addr [1] = {commands[1]};
       if(init(addr,1))
      {
        acom_sendCom("Error during init");
        return;
      }
      break;
    case 2222: //Turn on Mux channel
      Wire.beginTransmission(0x70);
      Wire.write(255); Wire.write(1 << commands[1]);
      muxOutput = Wire.endTransmission();

      if(debug){ acom_sendCom( "Mux Set: "+String(muxOutput) ); }
      break;
    case 3333: //Write registers to chips
      for(int iReg = 0; iReg<32; iReg++){
        Wire.beginTransmission(commands[1]);
        delay(2);
        Wire.write(iReg); 
        delay(2);
        Wire.write(regs[iReg]);
        delay(2);
        regOutput = Wire.endTransmission();

        if(debug){ acom_sendCom("Register Set ("+String(iReg)+"): "+String(regOutput) );}
      }
      break;
    case 4444:
      digitalWrite(13, LOW);
      digitalWrite(13, HIGH);

    float current=0; 
    String result = "Data: ";
    for (int i=0; i<4;i++)
    {
      readCurrent(commands[1], regs[i], current);  
      result += String(current);
      if (i < 3) result += ", ";
    }
    acom_sendCom(result);
    

      acom_sendCom("Done");
    default:
      break;
  }
}

bool acom_sendCom( String message ){
  delay(1000); //Wait 1 second for python to get in correct state
  Serial.println(message);
}

bool acom_receiveCom(){
  while( !Serial.available() ); //Wait to receive a command from Python
  String heard = Serial.readString();

  acom_sendCom( heard ); //Send heard command back to make sure it was correct

  while( !Serial.available() ); //Wait to receive a command from Python
  int confirm = Serial.readString().toInt();
  if(confirm==1){ 
    runCom(heard);
    acom_sendCom("Done");
  }
}

void loop() {
  digitalWrite(13, HIGH);
  acom_receiveCom();
  //digitalWrite(13, LOW);
}
