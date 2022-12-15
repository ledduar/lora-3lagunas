#!/bin/bash

mname="CLARO1"
port=5201
server="iperf.volia.net."
numPerBW=5  #simulaciones x BW

declare -a arrayBW

initbw=90000 #100kbps   init bw
finbw=2000000 #20mbps      fon BW
val=$initbw
arrayBW+=($val)

while [[ $finbw -ge $val ]]
    do
        while [[ 2000000 -ge $val ]] #<1mbps
        do
            let val=$val+400000 #+100kbps
            arrayBW+=($val)
        done
     
    echo "Valores a evaluar en iperf"
    echo "${arrayBW[@]}"
    done

npac=0
cont=0
now=$(date)
echo "$now" >> ./final_report_$mname.txt 2>&1
for i in "${arrayBW[@]}"
do
    rm ./Verbose*
    rm ./Count*
    echo "____________________________________________________________________________"
    echo "____________Evaluando con BW=$i bps, server $server ____________"
    echo "_____________________________________________________________________________"
    echo "Evaluando con BW=$i bps" >> ./final_report_$mname.txt 2>&1

    cont=0
    while [[ numPerBW -ge $cont ]] 
        do
            rm ./Verbose*
            rm ./Count*
            npac=0
            iperf3 -c $server -p $port -u -t 1 -b $i >> "./Verbosebw.txt" 2>&1
            grep ^"iperf Done." ./Verbosebw.txt > ./Count.txt
            npac=$(cat ./Count.txt | wc -l)
            if [ $npac  -eq  1 ]
            then
                let cont=$cont+1
                echo $(tail -n 3 ./Verbosebw.txt | head -n 1) >> ./final_report_$mname.txt 2>&1
                echo "_____+ Servidor ok test -> $cont +______"
            else
                echo "_____+ Servidor no responde +______"
            fi
            
        done
    
done
echo "_____+ FIN +_____"

