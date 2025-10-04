#include "SECONDARY_MEMORY.hpp"

SECONDARY_MEMORY::SECONDARY_MEMORY(size_t size) {
    if (size > MAX_SECONDARY_MEMORY_SIZE) {
        this->size = MAX_SECONDARY_MEMORY_SIZE;
    } else {
        this->size = size;
    }
    this->storage.resize(this->size, MEMORY_ACCESS_ERROR);
}

SECONDARY_MEMORY::~SECONDARY_MEMORY() {
    this->storage.clear();
}

// Simulação de acesso lento
uint32_t SECONDARY_MEMORY::ReadMem(uint32_t address) {
    if (address < this->size) {
        // Varredura simulada: percorre o vetor para encontrar o endereço
        for (uint32_t i = 0; i < this->size; ++i) {
            if (i == address) {
                return storage[i];
            }
        }
    }
    return MEMORY_ACCESS_ERROR;
}

// Simulação de acesso lento
uint32_t SECONDARY_MEMORY::WriteMem(uint32_t address, uint32_t data) {
    if (address < this->size) {
        // Varredura simulada: percorre o vetor para encontrar o endereço
        for (uint32_t i = 0; i < this->size; ++i) {
            if (i == address) {
                storage[i] = data;
                return data;
            }
        }
    }
    return MEMORY_ACCESS_ERROR;
}

uint32_t SECONDARY_MEMORY::DeleteData(uint32_t address) {
    if (address < this->size) {
        uint32_t deletedData = storage[address];
        storage[address] = MEMORY_ACCESS_ERROR;
        return deletedData;
    }
    return MEMORY_ACCESS_ERROR;
}

bool SECONDARY_MEMORY::isEmpty() {
    for (const auto &val : storage) {
        if (val != MEMORY_ACCESS_ERROR) return false;
    }
    return true;
}

bool SECONDARY_MEMORY::notFull() {
    for (const auto &val : storage) {
        if (val == MEMORY_ACCESS_ERROR) return true;
    }
    return false;
}