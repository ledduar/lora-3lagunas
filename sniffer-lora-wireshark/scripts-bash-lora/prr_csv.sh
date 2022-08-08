#!/bin/bash

for estacion in "tx" "rx"; do
    for p in 1 2; do
        files="$estacion/p$p/*.json"
        echo "$files"
        for file in $files; do
            echo "$file" 
            temporal="temporal.csv"
            comb=$(echo "$file" | cut -c9-)
            file_csv="resultados-ppr/${comb%???????}-$estacion.csv"
            jq -r '.data[] | [.sec] | @csv' $file >>$temporal
            muestra=$(cat $temporal | awk 'END{print NR}')
            resultado=$( echo "${file%?????},$muestra" | tr "/" ,)
            echo "$resultado"  >>$file_csv
            rm $temporal
        done
    done
done