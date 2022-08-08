#!/bin/bash

for estacion in "rx"; do
    for p in 1 2; do
        files="$estacion/p$p/*.json"
        echo "$files"
        for file in $files; do
            temporal="temporal.csv"
            comb=$(echo "$file" | cut -c9-)
            file_csv="resultados-rssi/${comb%???????}-$estacion.csv"
            jq -r '.data[] | [.rssi] | @csv' $file >>$temporal
            rssi=$(awk '{total+=$1;count++} END {print total/count}' $temporal)
            resultado=$( echo "${file%?????};$rssi" | tr "/" \; | tr "." ,)
            echo "$resultado"  >>$file_csv
            rm $temporal
        done
    done
done
