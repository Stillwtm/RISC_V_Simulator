#include <iostream>
#include "cpu.hpp"

//#define DEBUG

int main() {
#ifdef DEBUG
    std::string testCases[] = {"array_test1.data", "array_test2.data", "basicopt1.data", "bulgarian.data", "expr.data",
                                  "gcd.data", "hanoi.data", "lvalue2.data", "magic.data", "manyarguments.data",
                                  "multiarray.data", "naive.data", "pi.data", "qsort.data", "queens.data",
                                  "statement_test.data", "superloop.data", "tak.data"};
    for (auto& s : testCases) {
        std::ifstream in(R"(D:\Documents\C++ Projects\RISC-V-Simulator\testcases_for_riscv\testcases\)" + s);
        cpu cpuTest;
        cpuTest.init(in);
        std::cout << s << "\t";
        cpuTest.run();
    }
#endif
    cpu cpuTest;
    cpuTest.init(std::cin);
    cpuTest.run();

    return 0;
}
