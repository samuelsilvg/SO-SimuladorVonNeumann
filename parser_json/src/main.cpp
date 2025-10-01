#include <iostream>
#include "MainMemory.hpp"
#include "parser_json.hpp"

int main() {
    try {
        MainMemory ram;
        int endAddr = loadJsonProgram("tasks/tasks.json", ram, 0);
        std::cout << "Programa carregado até endereço: " << endAddr << "\n";
        ram.DumpMem();
    } catch (const std::exception &e) {
        std::cerr << "Erro: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
