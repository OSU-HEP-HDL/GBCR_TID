#include <Wire.h>

bool debug = true;

byte regs[32] = {8, 187, 187, 117, 8, 187, 187, 117, 8, 187, 187, 117, 8, 187, 187, 117, 8, 187, 187, 117, 8, 187, 187, 117, 23, 249, 100, 23, 249, 100, 33, 66};

void setup() { 
  Serial.begin(115200); 
  Serial.setTimeout(250);
  
  Wire.begin();
  Wire.setClock(100000);
  Wire.setWireTimeout(3000, 1);

  pinMode(13, OUTPUT);
}

void recordCurrents(int val, int thisPS){
  int psRegs[8] = {0x02, 0x0D, 0x10, 0x11, 0x14, 0x15, 0x18, 0x19};
  //int regs240[4] = {0x14, 0x15, 0x18, 0x19};
  //int regs015[4] = {0x0C, 0x0D, 0x10, 0x11};
  int data[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  for(int iData = 0; iData < 8; iData++){
      Wire.beginTransmission(thisPS);
      Wire.write(psRegs[iData]);
      Wire.requestFrom(thisPS, 1);
      int thisData = Wire.read();
      Wire.endTransmission();
      
      data[iData] = thisData;
   }
  
   String data1 = String(data[0], BIN)+"_"+String(data[1], BIN);
   String data2 = String(data[2], BIN)+"_"+String(data[3], BIN);
   String data3 = String(data[4], BIN)+"_"+String(data[5], BIN);
   String data4 = String(data[6], BIN)+"_"+String(data[7], BIN);

  acom_sendCom("Data: "+data1+", "+data2+", "+data3+", "+data4);
}

void readPS(int thisPS){
     Wire.beginTransmission(thisPS);
     Wire.write(0x01);
     Wire.write(0xFF);
     int wrote = Wire.endTransmission();

     delay(500);

     int count = 0;
     int n; int val;
     while(count < 10){
       Wire.beginTransmission(thisPS);
       Wire.write(0x00);
       n = Wire.requestFrom(thisPS, 1);
       val = Wire.read();
       Wire.endTransmission();
       //acom_sendCom("Requested data from PS: "+String(Wire.endTransmission()));
       acom_sendCom("Data from status register: "+String(val, BIN));
       if(val==255){break;}
       else{count++;}
     }

     if(debug){
      acom_sendCom( "Power Supply State Set: "+String(wrote) );
      //acom_sendCom("Number of Bytes read from PS: "+String(n));
      acom_sendCom("Data from status register: "+String(val, BIN));
     }

     //acom_sendCom(String(val));
     //int val = 240;
     if(val==255){recordCurrents(val, thisPS);}
     //if(val==240){ recordCurrents(val, thisPS); }
     //else if(val==15){ recordCurrents(val, thisPS); }
     else{ acom_sendCom("Retry"); }
     
     acom_sendCom("CDone");
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
      Wire.beginTransmission(commands[1]);
      Wire.write(0x06); Wire.write(17);
      w[0] = Wire.endTransmission();

      Wire.beginTransmission(commands[1]);
      Wire.write(0x07); Wire.write(17);
      w[1] = Wire.endTransmission();

      Wire.beginTransmission(commands[1]);
      Wire.write(0x08); Wire.write(16);
      w[2] = Wire.endTransmission();

      if(debug){ acom_sendCom("PS turned on: "+String(w[0])+", "+String(w[1])+", "+String(w[2]) ); }
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
        Wire.write(iReg); Wire.write(regs[iReg]);
        regOutput = Wire.endTransmission();

        if(debug){ acom_sendCom("Register Set ("+String(iReg)+"): "+String(regOutput) );}
      }
      break;
    case 4444:
      digitalWrite(13, LOW);
      digitalWrite(13, HIGH);
      readPS(commands[2]);
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
