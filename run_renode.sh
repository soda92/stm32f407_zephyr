#!/bin/bash
ELF_PATH="build/zephyr/zephyr.elf"

if [ ! -f "$ELF_PATH" ]; then
    echo "Error: $ELF_PATH not found. Run 'west build' first."
    exit 1
fi

# Run the dedicated Renode test setup
renode my_port/test_setup.resc
