from usb_iss import UsbIss, defs
import math, time

test = 6
skip = -1

usbPort = "/dev/ttyACM1"
muxTOboard = {2:2, 3:4, 4:5, 5:6, 6:3, 7:1}
muxOrder = [7, 2, 6, 3, 4, 5]

def testConnections():
    iss = UsbIss(); iss.open(usbPort)
    iss.setup_i2c(clock_khz=100, use_i2c_hardware=True, io1_type=None, io2_type=None)
    #print(iss.read_fw_version());
    #Check the multiplexer is on
    if(not iss.i2c.test(0x70)):
        print("Could not find the multiplexer")
        iss.close()
        exit()

    #Check all the channels are connected
    nConnected = 0
    for iMux in muxOrder:
        if(iMux==skip):
            continue
        thisMux = int(math.pow(2, iMux))
        try:
            iss.i2c.write(0x70, 0x00, [thisMux])
        except:
            print("Can't find chip channel: "+str(muxTOboard[iMux]))
            time.sleep(1)
            continue

        message = str(muxTOboard[iMux])+": "

        for iChip in range(4):
            passed = False
            thisChip = 32+iChip
            try:
                passed = iss.i2c.test(thisChip)
            except:
                passed = False

            if(passed):
                message+=str(hex(thisChip))+" "
            time.sleep(1)

        print(message)

    iss.close()
    
if __name__ == "__main__":
    testConnections()
