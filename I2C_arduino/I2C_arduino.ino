#include <Wire.h>

int regs[32] = {8, 187, 187, 117, 8, 187, 187, 117, 8, 187, 187, 117, 8, 187, 187, 117, 8, 187, 187, 117, 8, 187, 187, 117, 23, 249, 100, 23, 249, 100, 33, 66};
int psPower[3] = {17, 17, 16};
int receivedData[5] = { -50, -50, -50, -50, -50 };

void setup() { 
  Serial.begin(1000000); 
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

//void sendContinue(){
//  String message = "Acom_conintue";
//  Serial.println(message);
//  delay(100);
//
//  while( !Serial.available() );
//  int test = Serial.readString().toInt();
//  if(test == 0){ sendContinue(); }
//}
//

//
//void splitReceived( String received ){

//}
//
//void awaitForMessage(){
//  while( !Serial.available() );
//  
//  String received = Serial.readString();
//  splitReceived(received);
//}

bool checkMessage( String message ){
  bool correct = false;
  String send = "ACom_"+message;
  Serial.println(send);

  while( !Serial.available() );
  int check = Serial.readString().toInt();
  if( check==1 ){ correct = true; }

  return correct;
}

String receiveMessage(){
  String command = "";
  while( !Serial.available() );

  command = Serial.readString();
  bool passed = checkMessage( command );
  if( not passed ){ command = receiveMessage(); }

  return command;
}

void decodeCommand( String command ){
  int subIndex = 0;
  int index = 0;
  int rLength = command.length();

  while( subIndex < rLength ){
    String thisCode = command.substring(subIndex, subIndex+4);
    subIndex = subIndex + 4;

    receivedData[index] = thisCode.toInt();
    index = index+1;
  }
}

void resetForNextCommand(){
  for(int i=0; i<6; i++){ receivedData[i] = -50; }
  Serial.println("ACom_Continue");
}

void turnOnPS(){
  Wire.beginTransmission(receivedData[1]);
  Wire.write(6);
  for(int i=0; i<3; i++){ Wire.write( psPower[i] ); }
  Wire.endTransmission();
}

void turnOnMux(){
  Wire.beginTransmission(receivedData[1]);
  Wire.write(1 << receivedData[2]);
  Wire.endTransmission();
}

void registerChip(){
  Wire.beginTransmission(receivedData[1]);
  for(int i=0; i<32; i++){ Wire.write(regs[i]); }
  Wire.endTransmission();
}

void loop() {
  String command = receiveMessage();
  decodeCommand( command );
  //waitForMessage();

  int commandCode = receivedData[0];
  if(commandCode==4444){ turnOnPS(); }
  else if(commandCode==5555){ turnOnMux(); }
  else if(commandCode=6666){ registerChip(); }

  resetForNextCommand();
 
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
