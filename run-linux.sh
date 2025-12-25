#!/bin/bash
# NCL RISC-V Emulator Launcher (Linux)

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
EMU="$SCRIPT_DIR/bin/riscv-emu"

# Build if needed
if [ ! -f "$EMU" ]; then
    echo "Building emulator..."
    cd "$SCRIPT_DIR" && make
fi

# Try different terminal emulators
if command -v gnome-terminal &> /dev/null; then
    gnome-terminal --title="NCL RISC-V" -- "$EMU" "$@"
elif command -v xfce4-terminal &> /dev/null; then
    xfce4-terminal --title="NCL RISC-V" -e "$EMU $*"
elif command -v konsole &> /dev/null; then
    konsole --title "NCL RISC-V" -e "$EMU" "$@"
elif command -v xterm &> /dev/null; then
    xterm -title "NCL RISC-V" -e "$EMU" "$@"
else
    echo "No supported terminal found. Running in current terminal..."
    "$EMU" "$@"
fi
