#!/usr/bin/env bash

set -euo pipefail
IFS=$'\n\t'

echo "This script re-installs linamp executable files in linamp-os. This should only be run inside linamp-os"

# Assumes the build files are on the same folder as this script
rm -rf ~/linamp/*
cp *.o ~/linamp/
cp player ~/linamp/
cp shutdown.sh ~/linamp/
cp start.sh ~/linamp/
cp -r venv ~/linamp/

echo "Done!"
