#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# Pi panel: portrait HDMI output rotated to landscape 1280x400
if wlr-randr --output HDMI-A-1 &>/dev/null; then
    wlr-randr --output HDMI-A-1 --transform 270
# UTM / virtio display: landscape 1280x400 (no rotation)
elif wlr-randr --output Virtual-1 &>/dev/null; then
    if ! wlr-randr --output Virtual-1 2>/dev/null | grep -q '1280x400'; then
        wlr-randr --output Virtual-1 --custom-mode 1280x400@60 || true
    fi
fi

source "$SCRIPT_DIR/venv/bin/activate"
PYTHONPATH="$SCRIPT_DIR/python" "$SCRIPT_DIR/build/player"
