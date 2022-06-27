#include <iostream>
#include "cpu.h"

int main() {
    cpu cpuTest;
    std::ifstream in(R"(D:\Documents\C++ Projects\RISC-V-Simulator\testcases_for_riscv\testcases\queens.data)");
    cpuTest.init(in);
//    cpuTest.init(std::cin);
    cpuTest.run();

    return 0;
}
