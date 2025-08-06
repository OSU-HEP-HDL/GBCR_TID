#include <Wire.h>
#include<avr/wdt.h>

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
  int psRegs[8] = {0x0C, 0x0D, 0x10, 0x11, 0x14, 0x15, 0x18, 0x19};
  //int regs240[4] = {0x14, 0x15, 0x18, 0x19};
  //int regs015[4] = {0x0C, 0x0D, 0x10, 0x11};
  byte data[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  String data1 = "";
  String data2 = "";
  String data3 = "";
  String data4 = "";

 union
  {
    uint8_t b[2];
    uint16_t w;
  } myData;
  myData.b[0]=0;
  myData.b[1]=0;
  int address=0x4f;
  int reg = 0x18;

  byte thisData = 0;

  // throw away first data from tht LTC2991
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(address, 2, true);
  thisData = Wire.read();
  thisData = Wire.read();
  Wire.endTransmission();


  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(address, 2, true);
  myData.b[1] = Wire.read();
  Serial.print("B1: ");
  Serial.println(myData.b[1],HEX);
  myData.b[0] = Wire.read();
  Serial.print("B0: ");
  Serial.println(myData.b[0],HEX);
  Wire.endTransmission();
  uint16_t adc = myData.w;
 



  // for(int iData = 0; iData < 8; iData++){
  //     Wire.beginTransmission(thisPS);
  //     Wire.write(psRegs[iData]);
  //     Wire.requestFrom(thisPS, 1);
  //     byte thisData = Wire.read();
  //     Wire.endTransmission();
      
  //     data[iData] = thisData;
  //  }
  //  delay(100);
  
  //  data1 = String(data[0], BIN)+"_"+String(data[1], BIN);
  //  data2 = String(data[2], BIN)+"_"+String(data[3], BIN);
  //  data3 = String(data[4], BIN)+"_"+String(data[5], BIN);
  //  data4 = String(data[6], BIN)+"_"+String(data[7], BIN);
  // acom_sendCom("Data: "+data1+", "+data2+", "+data3+", "+data4);
  acom_sendCom("Data:"+String(adc, BIN));
}


void readPS(int thisPS){
     Wire.beginTransmission(thisPS);
     Wire.write(0x01);
     Wire.write(0xF8);
     int wrote = Wire.endTransmission();

     delay(500);

     int count = 0;
     int n; byte val;
     while(count < 10){
       Wire.beginTransmission(thisPS);
       Wire.write(0x00);
       Wire.endTransmission();
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
      commands[1]=0x4F;
      // Do some initialization
      Wire.beginTransmission(commands[1]);        
      Wire.write(0x01); Wire.write(0xF8);         //! Enables all channels
      Wire.endTransmission();
      Wire.beginTransmission(commands[1]);
      Wire.write(0x06); Wire.write(0x00);          //! Sets registers to default starting values.
      Wire.endTransmission();
      Wire.beginTransmission(commands[1]);
      Wire.write(0x07); Wire.write(0x00);
      Wire.endTransmission();
      Wire.beginTransmission(commands[1]);
      Wire.write(0x08); Wire.write(0x10);           //! Configures LTC2991 for Repeated Acquisition mode
      Wire.endTransmission();

      Wire.beginTransmission(commands[1]);
      Wire.write(0x06); Wire.write(0x11);
      w[0] = Wire.endTransmission();

      Wire.beginTransmission(commands[1]);
      Wire.write(0x07); Wire.write(0x11);
      w[1] = Wire.endTransmission();

      Wire.beginTransmission(commands[1]);
      Wire.write(0x08); Wire.write(0x10);
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
      //readPS(commands[1]);
//      readPS(0x4f);
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
