#!/usr/bin/env bash

set -euo pipefail
IFS=$'\n\t'

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd "$SCRIPT_DIR"

# Setup build dependencies (see README.md Development section)
sudo apt-get install build-essential cmake \
    qt6-base-dev qt6-base-dev-tools qt6-multimedia-dev qtcreator \
    libtag1-dev libasound2-dev libpulse-dev libpipewire-0.3-dev libdbus-1-dev \
    libcdio-dev libcdio-paranoia-dev libcdio-utils libdiscid-dev \
    python3-pip python3-full python3-dev wlr-randr -y

python3 -m venv venv
source venv/bin/activate
pip install -r python/requirements.txt
