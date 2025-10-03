#include "MAIN_MEMORY.hpp"

MAIN_MEMORY::MAIN_MEMORY(size_t size)
{
    if (size > MAX_MEMORY_SIZE)
        this->size = MAX_MEMORY_SIZE;
    else
        this->size = size;

    this->ram.resize(this->size, MEMORY_ACCESS_ERROR);
}

MAIN_MEMORY::~MAIN_MEMORY()
{
    this->ram.clear();
}

bool MAIN_MEMORY::isEmpty()
{
    for (auto &val : ram)
        if (val != MEMORY_ACCESS_ERROR) return false;
    return true;
}

bool MAIN_MEMORY::notFull()
{
    for (auto &val : ram)
        if (val == MEMORY_ACCESS_ERROR) return true;
    return false;
}

uint32_t MAIN_MEMORY::ReadMem(uint32_t address)
{
    if (address < this->size)
        return ram[address];
    return MEMORY_ACCESS_ERROR;
}

uint32_t MAIN_MEMORY::WriteMem(uint32_t address, uint32_t data)
{
    if (address < this->size)
    {
        ram[address] = data;
        return ram[address];
    }
    return MEMORY_ACCESS_ERROR;
}

uint32_t MAIN_MEMORY::DeleteData(uint32_t address)
{
    if (address < this->size && ram[address] != MEMORY_ACCESS_ERROR)
    {
        uint32_t deletedData = ram[address];
        ram[address] = MEMORY_ACCESS_ERROR;
        return deletedData;
    }
    return MEMORY_ACCESS_ERROR;
}
