#include <iostream>
#include "cpu.h"

int main() {
    cpu cpuTest;
    // not pass: expr.data, hanoi.data, magic.data,
    std::ifstream in(R"(D:\Documents\C++ Projects\RISC-V-Simulator\testcases_for_riscv\testcases\pi.data)");
    cpuTest.init(in);
    cpuTest.run();

    return 0;
}
