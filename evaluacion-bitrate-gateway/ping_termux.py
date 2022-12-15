#!/usr/bin/env python3
#import matplotlib.pyplot as plt

#import numpy as np
#import iperf3
import time
import statistics
#import random
from datetime import datetime
#from ping3 import ping, verbose_ping

import os 

Operadora = "CLARO"
repSize = 1            # Num repeticiones x size packet
numPackets = 20        # Num packetes a enviar x experimento 
ipDestino = " 1.1.1.1 "
interVal = 0.4            # seconds default 1s

initSize = 10            # size init MIN 10  google
finSize = 60           # size fin    MAX 68  google

arrayPckts = []           #array of packets size
vall = interVal

while vall < 6:

    if (vall <= 1):
        vall = vall + 0.1
        arrayPckts.append(round(vall,1))
    if (vall > 1 and vall <= 3):
        vall = vall + 0.5
        arrayPckts.append(round(vall,1))
    if (vall > 3 and vall <= 6):
        vall = vall + 1
        arrayPckts.append(round(vall,0))
   

    
       
print(arrayPckts)
time.sleep(5)

newpath = './'+Operadora+'Ping_Test'+'_At:'+str(datetime.now().hour) +':'+str(datetime.now().minute)+':'+str(datetime.now().second)
if not os.path.exists(newpath):
    os.makedirs(newpath)
    
lineHeader = 'Time' +'*'+ 'SizePacket' + '*' + '#Packets Tx' + '*' + '#Packets Rx' + '*' + 'LostPercent' + '*' + 'rttMax' + '*' +'rttMin' + '*' + 'rttAvg' + '*' + 'rttMde' + '*' + 'interval'
timeLog = str(datetime.now())
nameFig1 = 'Test_at_'+ timeLog
nameF2 = newpath+'/'+  nameFig1  + 'Verbose.txt'
f4 = open(nameF2,'w+')
nameF3 = newpath+'/'+  nameFig1  + 'Summary.txt'
f3 = open(nameF3,'w+')
f3.write(lineHeader)


arrayRttavg = []
cntPcksize = 0
while cntPcksize in range(len(arrayPckts)):
    
    
    arrayRtt_Rep = []
    cout = 0
    
    f4.write('\n Packet Size'+ str(arrayPckts[cntPcksize]) +'\n')
    while cout < repSize:
        f2 = open(nameF2,'w+')
        result = os.popen("ping -c "+ str(numPackets) + ' -i ' + str(arrayPckts[cntPcksize])+ ' -s ' + str(65)+' '+ipDestino)
        
        for line in result.readlines():
            f2.write(line)
            print(line)
            f4.write(str(line))
        
        f4.write('\n')

        f2.close()
        f2 = open(nameF2,'r+')
        cont = f2.readlines()
        readLine = cont[-2:]
        
        print(readLine)
        print(readLine[1].split()[3])
        #print(len(readLine[1].split()[3]))
        #print(len(readLine[0].split()))
        #print(readLine[0].split())
        a12=0;
        if(len(readLine[1].split()[3])):
            a12=1
        elif(len(readLine[0].split())):
            a12=1
        else:
            a12=0
        if(a12>0):
            if( (len(readLine[1].split()[3]) < 26) or (len(readLine[0].split()) < 10) ):
                valpackTx = '0'
                valpackRx = '0'
                valPecentlost = '100'
                valRTTmax = '0'
                valRTTmin = '0'
                valRTTavg = '0'
                valRTTmdev = '0'

            else:
                valpackTx = (readLine[0]).split()[0]    ##packets tx
                valpackRx = (readLine[0]).split()[3]    # packets Rx
                valPecentlost = (readLine[0]).split()[5] # % lost packets
                valRTTmax = (readLine[1].split()[3]).split('/')[2]
                valRTTmin = (readLine[1].split()[3]).split('/')[0]
                valRTTavg = (readLine[1].split()[3]).split('/')[1]
                valRTTmdev = (readLine[1].split()[3]).split('/')[3]
                arrayRtt_Rep.append(float(valRTTavg))
                cout+=1 
            

        
            

        f2.truncate(0)
        f2.close()

        lineData = '\n' + timeLog + '*' + str(65) +'*'+ valpackTx + '*' +  valpackRx + '*' +  valPecentlost + '*' +  valRTTmax +'*' +  valRTTmin + '*' + valRTTavg + '*' + valRTTmdev + '*' + str(arrayPckts[cntPcksize])
        f3.write(lineData)
        
        
        
    print(arrayRtt_Rep)
    arrayRttavg.append(statistics.mean(arrayRtt_Rep))
    cntPcksize+=1
    

print(arrayRttavg)
f3.close()
f4.close()


