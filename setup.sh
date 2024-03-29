#!/usr/bin/env bash

set -euo pipefail
IFS=$'\n\t'

# This setups python environment
sudo apt-get install vlc libiso9660-dev libcdio-dev libcdio-utils swig python3-pip python3-full python3-dev libdiscid0 -y

python3 -m venv venv
source venv/bin/activate
pip install -r python/requirements.txt
