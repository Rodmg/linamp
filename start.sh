#!/usr/bin/env bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

source venv/bin/activate
DISPLAY=:0 PYTHONPATH=python $SCRIPT_DIR/player
