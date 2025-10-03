#include "MemoryManager.hpp"

MemoryManager::MemoryManager(size_t mainMemorySize, size_t secondaryMemorySize) {
    mainMemory = std::make_unique<MAIN_MEMORY>(mainMemorySize);
    secondaryMemory = std::make_unique<SECONDARY_MEMORY>(secondaryMemorySize);
    mainMemoryLimit = mainMemorySize;
}

uint32_t MemoryManager::read(uint32_t address) {
    // Se o endereço for menor que o limite da RAM, acesse a memória principal.
    if (address < mainMemoryLimit) {
        return mainMemory->ReadMem(address);
    } 
    // Senão, acesse a memória secundária.
    else {
        // Ajusta o endereço para o espaço da memória secundária
        // Ex: endereço 1024 se torna o endereço 0 no disco.
        uint32_t secondaryAddress = address - mainMemoryLimit;
        return secondaryMemory->ReadMem(secondaryAddress);
    }
}

void MemoryManager::write(uint32_t address, uint32_t data) {
    // Se o endereço for menor que o limite da RAM, escreva na memória principal.
    if (address < mainMemoryLimit) {
        mainMemory->WriteMem(address, data);
    }
    // Senão, escreva na memória secundária.
    else {
        uint32_t secondaryAddress = address - mainMemoryLimit;
        secondaryMemory->WriteMem(secondaryAddress, data);
    }
}