#!/usr/bin/env bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

wlr-randr --output HDMI-A-1 --transform 270

source $SCRIPT_DIR/venv/bin/activate
DISPLAY=:0 PYTHONPATH=$SCRIPT_DIR/python $SCRIPT_DIR/build/player
