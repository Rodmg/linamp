#!/usr/bin/env bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

source $SCRIPT_DIR/venv/bin/activate
DISPLAY=:0 PYTHONPATH=$SCRIPT_DIR/python $SCRIPT_DIR/player
