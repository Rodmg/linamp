#!/usr/bin/env bash

set -euo pipefail
IFS=$'\n\t'

for filePath in skin/*.png; do
  if [ -f "$filePath" ]; then
    fileName="${filePath##*/}"
    echo "$fileName"
    convert -scale 400% "$filePath" "assets/$fileName"
  fi
done

