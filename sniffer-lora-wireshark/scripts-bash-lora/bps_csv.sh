#!/bin/sh

files="pcap/*.pcap"
for file in $files; do
    comb=$(echo "${file%???????}" | cut -c14-)
    estacion=$(echo "${file}" | cut -c6-7)
    file_csv="resultados-bps/$comb-$estacion.csv"
    bps=$(capinfos -i -T -r $file)
    resultado=$(echo $bps | tr "-" \; | tr [:space:] \;)
    echo "$resultado" >> $file_csv
    sed -i 's/pcap\///' $file_csv
    sed -i 's/.pcap//' $file_csv
done