#!/usr/bin/env bash

set -euo pipefail
IFS=$'\n\t'

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd "$SCRIPT_DIR"

# This setups python environment
sudo apt-get install build-essential vlc libiso9660-dev libcdio-dev libcdio-utils swig python3-pip python3-full python3-dev libdiscid0 -y

python3 -m venv venv
source venv/bin/activate
pip install -r python/requirements.txt
