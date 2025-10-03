#ifndef MEMORY_MANAGER_HPP
#define MEMORY_MANAGER_HPP

#include <memory>
#include <stdexcept>
#include "MAIN_MEMORY.hpp"
#include "SECONDARY_MEMORY.hpp"

// Define um tamanho padrão para a memória principal (RAM).
// Você pode ajustar este valor conforme a necessidade do simulador.
// Por exemplo, 1024 palavras de 32 bits.
const size_t MAIN_MEMORY_SIZE = 1024;

class MemoryManager {
public:
    // O construtor inicializa a memória principal e secundária com seus respectivos tamanhos.
    MemoryManager(size_t mainMemorySize, size_t secondaryMemorySize);

    // Métodos unificados para leitura e escrita que a CPU usará.
    uint32_t read(uint32_t address);
    void write(uint32_t address, uint32_t data);

private:
    std::unique_ptr<MAIN_MEMORY> mainMemory;
    std::unique_ptr<SECONDARY_MEMORY> secondaryMemory;
    size_t mainMemoryLimit; // Guarda o tamanho da RAM para saber onde começa o disco
};

#endif // MEMORY_MANAGER_HPP