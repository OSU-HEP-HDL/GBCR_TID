import pyvisa, serial, time, argparse

parser = argparse.ArgumentParser(prog='PScontroller',
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument("-p", "--power", required=True, default=0, type=int, help="Turn on power supply?", choices=[0,1])
parser.add_argument("-v", "--volts", required=False, default=-1, help="Voltage limit")
parser.add_argument("-i", "--current", required=False, default=-1, help="Current limit")
args = parser.parse_args()

instName = 'ASRL/dev/ttyUSB0::INSTR'
serNum = b"++addr 5\n" 

def turnOn(ser, inst):
    channels = ["OUTP1", "OUTP2"]
    ser.write(serNum); time.sleep(0.5)

    match args.power:
        case 0:
            inst.write('OUTP OFF'); time.sleep(0.5)
        case 1:
            for thisChan in channels:
                channelSel = "INST:SEL "+thisChan
                inst.write(channelSel); time.sleep(0.5)
                inst.write("APPL "+args.volts+", "+args.current); time.sleep(0.5)
                inst.write('OUTP ON'); time.sleep(0.5)

if __name__ == "__main__":
    if(args.power==1):
        if( (args.volts==-1) or (args.current==-1)):
            print("Please enter a voltage (-v) and current (-i)!")
            exit()

    rm = pyvisa.ResourceManager()
    inst = rm.open_resource(instName)
    ser = ""

    if(instName=='ASRL/dev/ttyUSB0::INSTR'):
        inst.baud_rate = 9600
        ser = serial.Serial('/dev/ttyUSB0')

    turnOn(ser, inst)
