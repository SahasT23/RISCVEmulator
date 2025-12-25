@echo off
REM NCL RISC-V Emulator Launcher (Windows)

set SCRIPT_DIR=%~dp0
set EMU=%SCRIPT_DIR%bin\riscv-emu.exe

REM Build if needed
if not exist "%EMU%" (
    echo Building emulator...
    cd /d "%SCRIPT_DIR%"
    mingw32-make
)

REM Open new command window with title
start "NCL RISC-V" cmd /k "%EMU%" %*
