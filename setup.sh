#!/usr/bin/env bash

set -euo pipefail
IFS=$'\n\t'

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd "$SCRIPT_DIR"

# Setup build dependencies for native CD playback and Python environment for BT/Spotify
sudo apt-get install build-essential \
    libcdio-dev libcdio-paranoia-dev libdiscid-dev \
    python3-pip python3-full python3-dev -y

python3 -m venv venv
source venv/bin/activate
pip install -r python/requirements.txt
