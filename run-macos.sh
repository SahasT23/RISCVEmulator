#!/bin/bash
# NCL RISC-V Emulator Launcher (macOS)

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
EMU="$SCRIPT_DIR/bin/riscv-emu"

# Build if needed
if [ ! -f "$EMU" ]; then
    echo "Building emulator..."
    cd "$SCRIPT_DIR" && make
fi

# Open new Terminal window
osascript <<EOF
tell application "Terminal"
    activate
    do script "cd '$SCRIPT_DIR' && '$EMU' $*; exit"
    set custom title of front window to "NCL RISC-V"
end tell
EOF
