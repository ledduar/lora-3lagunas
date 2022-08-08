#!/bin/bash

for estacion in "tx" "rx"; do
    for p in 1 2; do
        files="$estacion/p$p/*.json"
        echo "$files"
        for file in $files; do
            comb=$(echo "$file" | cut -c7-)
            file_csv="resultados-delay/p$p/${comb%???????}-$estacion.csv"
            echo "              " >>$file_csv
            echo "$file" >>$file_csv
            echo "$file"
            jq -r '.data[] | [.sec, .payload] | @csv' $file >>$file_csv
        done
    done
done
