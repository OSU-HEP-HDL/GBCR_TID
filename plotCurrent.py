import pylab as pl
import glob

timeGap1 = 2 #minutes
timeGap2 = 4 #minutes

def getData(inFiles):
    currentData = {}

    for iFile in inFiles:
        thisFile = open(iFile, 'r')
        psString = iFile.split('/')[-1][:-4]
        allData = thisFile.readlines()
        allData = [thisData.strip() for thisData in allData]

        t = []
        currents = [[], [], [], []]
        for iD, thisData in enumerate(allData):
            if(iD<100):
                t.append(iD*timeGap1)
            else:
                t.append(iD*timeGap2)
            splitData = thisData.split(',')
            for iS,thisSplit in enumerate(splitData):
                thisSplitCurrent = thisSplit.split(' - ')
                currents[iS].append(float(thisSplitCurrent[1]))
        currentData[psString] = [t, currents]
        thisFile.close()
    
    return currentData

def plotCurrents(currentData):
    psNames = currentData.keys()

    for thisPS in psNames:
        t = currentData[thisPS][0]
        tArray = pl.array(t)
        currents = currentData[thisPS][1]
        for iC in range(len(currents)):
            thisCurrent = currents[iC]
            if(thisCurrent[0]<0.01):
                continue

            iArray = pl.array(thisCurrent)
            pl.plot(tArray, iArray, '.b')

            pl.ylabel('Current (A)')
            pl.ylim(0, iArray[-1]*2)
            
            pl.xlabel('Time (min)')
            pl.xlim(0, tArray[-1]+2)

            thisName = thisPS+"_I"+str(iC)+".pdf"
            pl.savefig(thisName)

if __name__ == "__main__":
    currentFiles = glob.glob("./0x*")
    currentData = getData(currentFiles)

    plotCurrents(currentData)
