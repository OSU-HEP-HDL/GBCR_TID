import serial, time

arduino = serial.Serial(port="/dev/ttyACM0", baudrate=115200, timeout = 0.1)

def confirmReceived(sentVal):
    correctReceive = True
    while(not correctReceive):
        receivedMessage = arduino.readline().decode('utf-8')
        splitMessage = receivedMessage.split('_')
        splitVal = splitMessage[2:]
        receivedVal = '_'.join(splitVal)
        print(sentVal+" : "+receivedVal)
        
        if(splitMessage[0]!="Acom"):
            print("Pcom_FAILED")
            arduino.write( byte("Pcom_FAILED", 'utf-8') )
        else:
            print("Pcom_CORRECT")
            arduino.write( byte("Pcom_CORRECT", 'utf-8') )
            correctReceive = False

def testSend():
    val = input("Enter number: ")
    message = "Pcom_"+val
    
    arduino.write( bytes(message, 'utf-8') )
    #confirmRecieved(val)

if __name__ == "__main__":
    testSend()
    arduino.close()
