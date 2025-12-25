# RISC-V Emulator

This is a simple terminal RISC-V emulator that covers the base instructions for a 32 bit processor with hazards, forwarding and pipelining as well as using the ```mul``` extension as well.

Options to run the emulator with out hazards and/or forwarding and/or pipelining.

You can choose the single-cycle processor or the 5-stage processor.

```
riscv-emulator/
├── include/
│   ├── common.hpp
│   ├── memory.hpp
│   ├── register_file.hpp
│   ├── alu.hpp
│   ├── decoder.hpp
│   ├── assembler.hpp
│   ├── cpu.hpp
│   ├── pipeline.hpp
│   ├── hazard_unit.hpp
│   └── emulator.hpp
├── src/
│   ├── main.cpp
│   ├── memory.cpp
│   ├── register_file.cpp
│   ├── alu.cpp
│   ├── decoder.cpp
│   ├── assembler.cpp
│   ├── cpu.cpp
│   ├── pipeline.cpp
│   ├── hazard_unit.cpp
│   └── emulator.cpp
├── examples/
│   ├── factorial.asm
│   ├── fibonacci.asm
│   └── hazard_demo.asm
├── Makefile
└── README.md
```