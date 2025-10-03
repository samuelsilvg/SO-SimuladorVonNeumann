#include <iostream>
#include "cpu/ULA.hpp"

int main() {
    ALU alu;
    alu.execute(ADD, 10, 20, 0);
    std::cout << "Resultado do ADD: " << alu.result << ", Overflow: " << alu.overflow << std::endl;
    return 0;
}