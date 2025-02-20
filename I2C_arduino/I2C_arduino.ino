#include <Wire.h>

int regs[32] = {8, 187, 187, 117, 8, 187, 187, 117, 8, 187, 187, 117, 8, 187, 187, 117, 8, 187, 187, 117, 8, 187, 187, 117, 23, 249, 100, 23, 249, 100, 33, 66};
int receivedData[5] = { -50, -50, -50, -50, -50 };

void setup() { 
  Serial.begin(115200); 
  Serial.setTimeout(1);
  
  Wire.begin();
  Wire.setClock(100000);
  Wire.setWireTimeout(500000);
} 

//int initiateCommunication( String val ){
//  String message = "Acom - "+val;
//  Serial.println(message);
//  delay(100);
//  
//  
//  int test = Serial.readString().toInt();
//  while(test!="Pcom"){
//    Serial.println(test);
//    test = initiateCommunication( val );
//  }
//  return test;
//}

void splitReceived( int received ){
  Serial.println(received);
  int index = 0;
  while( received>0 ){
    int thisCode = received%10000;
    receivedData[index] = thisCode;
    received /= 10000;
    Serial.println(received);
  }
}

int awaitForMessage(){
  while( !Serial.available() );
  int received = Serial.readString().toInt();
  splitReceived( received );
  
  return received;
}

void loop() {
  int received = awaitForMessage();

  for(int i=0; i<5; i+=1){
    Serial.println(receivedData[i]);
  }

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
