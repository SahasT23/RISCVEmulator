/**
 * main.cpp
 * 
 * Entry point for the RISC-V emulator.
 */

#include "emulator.hpp"

int main(int argc, char* argv[]) {
    Emulator emu;

    // If a file is provided as argument, load it
    if (argc > 1) {
        emu.load(argv[1]);
    }

    // Run the interactive command loop
    emu.run();

    return 0;
}
