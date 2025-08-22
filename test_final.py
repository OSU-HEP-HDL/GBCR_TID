import serial, time,  inputimeout, datetime, influxdb_client

from influxdb_client.client.write_api import SYNCHRONOUS

usbPort = "/dev/ttyACM1"
arduino = serial.Serial(port=usbPort, baudrate=115200, timeout=5)

ps1, ps2, ps3, ps4 = 0x4F, 0x4B, 0x4E, 0x4D
muxAdr = 0x70
#muxCh2, muxCh3, muxCh5, muxCh6, muxCh7 = 3, 4, 6, 7, 8
muxCh2, muxCh3, muxCh5, muxCh6, muxCh7 = 2, 3, 5, 6, 7

sessionMap = { ps1: { muxCh6: [0x23], muxCh5: [0x22]},
               ps2: { muxCh7: [0x23, 0x20]},
               ps3: { muxCh2: [0x23, 0x20]},
               ps4: { muxCh3: [0x23, 0x20]},
              }

# sessionMap = { ps1: { muxCh6: [0x23] } } #Failed
# sessionMap = { ps2: { muxCh7: [0x23] }}

r_sense = 0.1
currentData = ""
url, token, org = "", "", ""

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


# def calculateCurrents():
#     global currentData

#     currents = []
#     modData = currentData.replace("Data: ", "")
#     splitData = modData.split(', ')
#     for thisCount in splitData:
#         splitData = thisCount.split("_")
        
#         fullData = splitData[0].zfill(8)+splitData[1].zfill(8)

#         newData = int(fullData[0])
#         # if(newData!=1):
#         #     continue
#         sign = int(fullData[1])
        
#         count = fullData[2:]
#         total = 0
#         print(fullData)
#         maxBit = len(count)
#         for i in range(maxBit):
#             if(count[i]=="1"):
#                 total += 2**(maxBit-1-i)

#         current = 0
#         if(sign==0):
#             current=(total*(19.075*1e-6))/r_sense
#         if(sign==1):
#             current=((total+1)*(-19.075*1e-6))/r_sense

#         currents.append(current)
#     return currents
            
    #     if(len(splitData))
    #     if(len(thisCount)!=8):
    #         continue
    #     print(thisCount)
    #     print(thisCount[0])
    # print(splitData) 

def pcom_checkCommand(sentMessage):
    receivedCorrect = False
    received = arduino.readline().decode('utf-8').strip()
    print(f"ACom: {received}")
    if(received==sentMessage):
        return True
    else:
        return False

    return receivedCorrect

def pcom_sendConfirm(correct):
    message = "0"
    if(correct):
        message = "1"
    print(f"PCom: {message}")
    arduino.write( bytes(message, 'utf-8'))

def waitForComDone():
    done = False
    while(not done):
        received = arduino.readline().decode('utf-8').strip()
        print(f"ACom: {received}")
        if(received == "Done"):
            done = True

def receiveCurrents():
    global currentData

    currentsDone = False
    while(not currentsDone):
        thisVal = arduino.readline().decode('utf-8').strip()
        if("Data: " in thisVal):
            currentData = thisVal.strip()
        print(f"ACom: {thisVal}")
        if(thisVal == "CDone"):
            currentsDone = True

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
            
def sendCom(comCode=-1, comDest=-1, directMessage = ""):
    time.sleep(1) #Give a 1 second buffer for arduino to get into correct state
    comDone = False
    
    ### Send inital command
    com = str(comCode)+str(comDest).zfill(4)#+str(muxAdr).zfill(4)+str(comCode)
    if(directMessage!=""):
        com = directMessage.zfill(16)
    print(f"PCom: {com}")
    arduino.write( bytes(com, 'utf-8') )

    ### Check command was received correctly
    comCheck = pcom_checkCommand(com)
    
    ### Send confirmation
    pcom_sendConfirm(comCheck)
    if(comCheck):
        ### Wait for currents
        if(comCode==4444):
            receiveCurrents()
        waitForComDone()
        comDone = True

    print("\n")
        
    return comDone

def runPS(thisPS):
    global currentData
    
    muxMap = sessionMap[thisPS]
    muxs = muxMap.keys()

    psON = False
    while(not psON):
        psON = sendCom(comCode = 1111, comDest = thisPS) #Turns on PS
    for iM, thisMux in enumerate(muxs):
        chips = muxMap[thisMux]
        # if(iM > 0):
        #     arduino.close()
        #     exit()
        for iC, thisChip in enumerate(chips):
            muxSet = False
            while(not muxSet):
                muxSet = sendCom(comCode = 2222, comDest = thisMux)

            regsSet = False
            while(not regsSet):
                regsSet = sendCom(comCode = 3333, comDest = thisChip)

            currentRead = False
            while(not currentRead):
                currentRead = sendCom(comCode = 4444, comDest = thisPS)
                currentRead = True
                # if(currentData==""):
                #     currentRead = False
            #currents = calculateCurrents()
            #currentData = ""
            #print(f"MEASURED CURRENTS FOR ({thisPS}): {currents}")

def runStudy():
    sessionPSlist = sessionMap.keys()
    count = 1
    endSession = ""
    waitTime = 300

    while(endSession!="y"):
        if( (count>3) and (waitTime == 300)):
            waitTime = 1200    
        for iP, thisPS in enumerate(sessionPSlist):
            # if(iP>0):
            #     arduino.close()
            #     exit()
            
            runPS(thisPS)
            recordCurrents(currentData, thisPS)

        try:
            ansTime = waitTime - 5
            endSession = inputimeout.inputimeout(prompt = "End Session (Y/N): ", timeout = ansTime).lower()
        except:
            endSession="n"
        count+=1
        if(endSession!="y"):
            time.sleep(5)
    
if __name__ == "__main__":
    #testSend()
    readENV()
    runStudy()
    arduino.close()

    
