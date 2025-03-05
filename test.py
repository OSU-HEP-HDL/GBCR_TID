import serial, time

ps1, ps2, ps3, ps4 = 0x4F, 0x4B, 0x4E, 0x4D
muxAdr = 0x70
muxCh2, muxCh3, muxCh5, muxCh6, muxCh7 = 3, 4, 6, 7, 8

sessionMap = { ps1: { muxCh6: [0x23], muxCh5: [0x22]},
               ps2: { muxCh7: [0x23, 0x20]},
               ps3: { muxCh2: [0x23, 0x20]},
               ps4: { muxCh3: [0x23, 0x20]},
              }

usbPort = "/dev/ttyACM0"
arduino = serial.Serial(port=usbPort, baudrate=1000000, timeout=10)

def standby():
    count = 0
    cont = False
    while( not cont ):
        count+=1
        print("In Standby")
        message = arduino.readline().decode('utf-8').strip()
        print(message)
        
        if(message == "ACom_Continue"):
            cont = True

        if(count == 5):
            arduino.close()
            exit()
            
def sendCommand(code, dest):
    command = ""
    match code:
        case 4444:
            command = "4444"+str(dest).zfill(4)+"4444"
        case 5555:
            command = "5555"+str(muxAdr).zfill(4)+str(dest).zfill(4)+"5555"
        case 6666:
            command = "6666"+str(dest).zfill(4)+"6666"

    arduino.write( bytes(command, 'utf-8') )
    message = arduino.readline().decode('utf-8').strip()
    print(message)
    checkMessage = "ACom_"+command
    print(checkMessage)

    if(checkMessage == message):
        arduino.write( bytes("1", 'utf-8') )
    else:
        arduino.write( bytes("0", 'utf-8') )
        time.sleep(2)
        sendCommand(code, dest)

    standby()
    time.sleep(1)
        
def runPS(thisPS):
    muxMap = sessionMap[thisPS]
    muxs = muxMap.keys()
    
    sendCommand(4444, thisPS)#Turns on PS
    for iM, thisMux in enumerate(muxs):
        chips = muxMap[thisMux]
        print(chips)
        if(iM > 0):
            arduino.close()
            exit()

        sendCommand(5555, thisMux)#Turns on Mux channel
        for iC, thisChip in enumerate(chips):
            sendCommand(6666, thisChip)#Write registers to chip
    

def runStudy():
    sessionPSlist = sessionMap.keys()
    for iP, thisPS in enumerate(sessionPSlist):
        if(iP>0):
            arduino.close()
            exit()
            
        runPS(thisPS)
    
if __name__ == "__main__":
    #testSend()
    runStudy()
    arduino.close()

    
