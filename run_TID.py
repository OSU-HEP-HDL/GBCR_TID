from usb_iss import UsbIss, defs
import math, time, inputimeout, datetime

import influxdb_client
from influxdb_client.client.write_api import SYNCHRONOUS

import serial

usbPort = "/dev/ttyACM1"

ps1, ps2, ps3, ps4 = 0x4F, 0x4B, 0x4E, 0x4D
muxAdr = 0x70
mux2, mux3, mux5, mux6, mux7 = 3, 4, 6, 7, 8

session = { ps1: { mux6:[0x23], mux5:[0x22] },
            ps2: { mux7:[0x23, 0x20] },
            ps3: { mux2:[0x23, 0x20] },
            ps4: { mux3:[0x23, 0x20] }
           }
channelMap = {2:2, 3:4, 4:5, 5:6, 6:3, 7:1}
r_sense = 0.1

url, token, org = "", "", ""
#token = ""
#org = ""

arduino = serial.Serial(port="/dev/ttyACM0", baudrate=1000000, timeout = 0.1)

def getFileName(saveFiles, thisPS):
    fileName = str(hex(thisPS))+"_"
    fileSaved = False
    for thisFile in saveFiles:
        if(fileName in thisFile):
            fileName = thisFile
            fileSaved = True
    if(not fileSaved):
        thisDate = datetime.date.today().strftime('%Y%m%d')+".txt"
        fileName+=thisDate
        saveFiles.append(fileName)

    return fileName

def writeRegisters(iss, thisPS, thisMux, thisChip):
    gbcr_registers =[0x8,0xBB,0xBB,0x75,
                     0x8,0xBB,0xBB,0x75,
                     0x8,0xBB,0xBB,0x75,
                     0x8,0xBB,0xBB,0x75,
                     0x8,0xBB,0xBB,0x75,
                     0x8,0xBB,0xBB,0x75,
                     0x17,0xF9,
                     0x64,0x17,0xF9,0x64,
                     0x21,0x42]

    #Turn on the power supply
    try:
        iss.i2c.write(thisPS, 0x6, [17, 17, 16])
    except:
        return False
    time.sleep(0.25)
    
    #Turn on MUX to write to GBCRs
    try:
        iss.i2c.write(0x70, 0xFF, [thisMux])
    except:
        return False
    time.sleep(0.25)
    
    nFails = 0
    iReg = 0
    while(iReg<len(gbcr_registers)):
        try:
            iss.i2c.write(thisChip, iReg, [gbcr_registers[iReg]])
            iReg+=1
        except:
            nFails+=1
            iReg = 0

        if(nFails==5):
            return False
        
        time.sleep(0.5)

    return True

def calcVoltages(ps_data):
    adds = [12, 16, 20, 24]
    voltages = []

    for thisAdd in adds:
        thisVoltage = -1
        thisData = ps_data[thisAdd]; nextData = ps_data[thisAdd+1]
        if( (thisData>>6)&1 == 0):
            thisVoltage = 19.075*((thisData&0x3F)<<8 | nextData)*1e-06
        else:
            thisVoltage = (-19.075)*(((thisData&0x3F)<<8 | nextData))*1e-06

        voltages.append(thisVoltage)

    return voltages
    
def calcCurrents(iss, thisPS):
    currents = []
    
    #Activate PS to get current information
    iss.i2c.write(thisPS, 0x1, [240])
    time.sleep(0.25)
    status = 0
    while(status!=255):
        time.sleep(2)
        psData = iss.i2c.read(thisPS, 0x0, 30)
        status = psData[0]

    voltages = calcVoltages(psData)
    currents = [thisV/r_sense for thisV in voltages]

    return currents

def recordCurrents(currents, thisPS):
    global url, token, org
    
    outName = "Currents_"+str(hex(thisPS))+".txt"
    outFile = open(outName,'a+')
    writeString = "Time = "+datetime.datetime.now().strftime('%H.%M.%S')+" "

    client = influxdb_client.InfluxDBClient(url=url, token=token, org=org)
    write_api = client.write_api(write_options = SYNCHRONOUS)

    for iC, thisCurrent in enumerate(currents):
        iString = "I"+str(iC+1)
        writeString+=iString+" = "+str(thisCurrent)+" "

        psString = str(hex(thisPS))
        p = influxdb_client.Point("Currents").tag("location", psString).field(iString, thisCurrent)
        write_api.write(bucket="testTID", org=org, record=p)

    writeString+="\n"
    outFile.write(writeString)
    outFile.close()

def recordIfWritten(chipWritten):
    outFile = open("chipsWritten.txt", 'a+')

    client = influxdb_client.InfluxDBClient(url=url, token=token, org=org)
    write_api = client.write_api(write_options = SYNCHRONOUS)

    writeString = ""
    for thisChip in chipWritten.keys():
        splitChip = thisChip.split('_')
        loc = "Spot "+splitChip[0]
        isWritten = 0
        writeString+=thisChip+" = "
        if(chipWritten[thisChip]):
            writeString+="1 "
            isWritten = 1
        else:
            writeString+="0 "

        p = influxdb_client.Point("Chips Written").tag("location", loc).field(splitChip[1], isWritten)
        write_api.write(bucket="testTID", org=org, record=p)

    writeString+="\n"

    outFile.write(writeString)
    outFile.close()

def readENV():
    global url, token, org

    inFile = open(".env", 'r')
    allLines = inFile.readlines()
    inFile.close()

    for thisLine in allLines:
        splitLine = thisLine.split(" = ")
        if(splitLine[0]=="url"):
            url = splitLine[-1].strip()
        elif(splitLine[0]=="token"):
            token = splitLine[-1].strip()
        elif(splitLine[0]=="org"):
            org = splitLine[-1].strip()
        else:
            print("Option not available")

def waitContinue():
    received = False
    while(not received):
        time.sleep(1)
        message = arduino.readline().decode('utf-8')
        #print(message)

        if(message=="Acom_continue"):
            arduino.write( bytes("1", 'utf-8') )
            received = True
        else:
            arduino.write( bytes("0", 'utf-8') )

def turnOnMuxChannel(thisMux):
    print(thisMux)
    commands = "5555"+str(thisMux).zfill(4)+str(thisMux).zfill(4)+"5555"
    arduino.write( bytes(commands, "utf-8") )
    waitContinue()
            
def runStudy():
    count = 1
    endSession = ""
    waitTime = 300

    while(endSession!="y"):
        if( (count>3) and (waitTime == 300)):
            waitTime = 1200
            
        powerSupplies = session.keys()
        for thisPS in powerSupplies:
            muxes = session[thisPS].keys()
            for thisMux in muxes:
                print("Starting Loop")
                turnOnMuxChannel(thisMux)
        #     chipWritten = {}
        #     for thisMux in muxes:
        #         chips = psData[thisMux]
        #         for thisChip in chips:
        #             muxNumber = int(math.log2(thisMux))
        #             wrote = True #writeRegisters(iss, thisPS, thisMux, thisChip)
        #             chipString = str(channelMap[muxNumber])+"_"+str(hex(thisChip))
        #             chipWritten[chipString] = wrote

        #     print(chipWritten)
        #     currents = []
        #     currents = calcCurrents(iss, thisPS)
        #     recordCurrents(currents, thisPS)
        #     recordIfWritten(chipWritten)
                
        #     iss.close()
            
        try:
            ansTime = waitTime - 5
            endSession = inputimeout.inputimeout(prompt = "End Session (Y/N): ", timeout = ansTime).lower()
        except:
            endsession="n"

        count+=1
        if(endSession!="y"):
            time.sleep(5)
            
if __name__ == "__main__":
    readENV()
    runStudy()
