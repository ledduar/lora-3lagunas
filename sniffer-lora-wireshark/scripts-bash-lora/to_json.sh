#!/bin/sh

for estacion in "tx" "rx"; do
  for punto in 1 2; do

    filesTXT="$estacion/p$punto/*.TXT"
    for file in $filesTXT; do
      mv -v "$file" "$(echo $file | tr '[A-Z]' '[a-z]')"
    done

    filestxt="$estacion/p$punto/*.txt"
    for file in $filestxt; do
      mv -- "$file" "${file%.txt}.json"
    done

    filesjson="$estacion/p$punto/*.json"
    for file in $filesjson; do
      sed -i '1i { "data": [' "$file" &&
        echo ']}' >>"$file"
      sed -i 's/,]}/]}/' $file
    done

  done
done
