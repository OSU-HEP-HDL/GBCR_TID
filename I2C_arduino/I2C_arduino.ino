#include <Wire.h>

int chip;
int regs[32] = {8, 187, 187, 117, 8, 187, 187, 117, 8, 187, 187, 117, 8, 187, 187, 117, 8, 187, 187, 117, 8, 187, 187, 117, 23, 249, 100, 23, 249, 100, 33, 66};
int sent;

void setup() { 
  Serial.begin(115200); 
  Serial.setTimeout(1);
  
  Wire.begin();
  Wire.setClock(100000);
  Wire.setWireTimeout(500000);
} 

int initiateCommunication( String val ){
  String message = "Acom - "+val;
  Serial.println(message);
  delay(100);
  while(!Serial.available());
  
  int test = Serial.readString().toInt();
  while(test!="Pcom"){
    Serial.println(test);
    test = initiateCommunication( val );
  }
  return test;
}

void loop() {

  int commWorking = initiateCommunication("Test");
  Serial.println("Here");
  
  
  
  //int test = Serial.readString().toInt();
  //Serial.println(test);
  //chip = Serial.readString().toInt();

  //Wire.beginTransmission(chip);
  //for(int iReg=0; iReg<33; ++iReg){
  //  Wire.write(iReg);
  //}
  //Wire.write(8);
  //sent = Wire.endTransmission();
  //Serial.println(sent);

  //delay(100);
  //Serial.read();
} 
