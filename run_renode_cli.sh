#!/bin/bash
# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
ELF_PATH="build/zephyr/zephyr.elf"

if [ ! -f "$ELF_PATH" ]; then
    echo "Error: $ELF_PATH not found. Run 'west build' first."
    exit 1
fi

# Run Renode in CLI mode
# Use @ prefix for the path as it's the Renode convention
renode --disable-xwt --plain -e "i @$SCRIPT_DIR/test_setup.resc"
