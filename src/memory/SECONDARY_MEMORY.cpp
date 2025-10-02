#include "SECONDARY_MEMORY.hpp"

SECONDARY_MEMORY::SECONDARY_MEMORY(size_t size)
{
    if (size > MAX_SECONDARY_MEMORY_SIZE)
        this->size = MAX_SECONDARY_MEMORY_SIZE;
    else
        this->size = size;

    this->rowSize = static_cast<size_t>(std::sqrt(MAX_SECONDARY_MEMORY_SIZE));
    this->storage.resize(rowSize, vector<uint32_t>(rowSize, MEMORY_ACCESS_ERROR));
}

SECONDARY_MEMORY::~SECONDARY_MEMORY()
{
    this->storage.clear();
}

bool SECONDARY_MEMORY::isEmpty()
{
    for (auto &row : storage)
        for (auto &val : row)
            if (val != 0) return false;
    return true;
}

bool SECONDARY_MEMORY::notFull()
{
    for (auto &row : storage)
        for (auto &val : row)
            if (val == 0) return true;
    return false;
}

inline size_t SECONDARY_MEMORY::getRow(uint32_t address) const
{
    return address / rowSize;
}

inline size_t SECONDARY_MEMORY::getCol(uint32_t address) const
{
    return address % rowSize;
}

uint32_t SECONDARY_MEMORY::ReadMem(uint32_t address)
{
    if (address < this->size)
        return storage[getRow(address)][getCol(address)];
    return MEMORY_ACCESS_ERROR;
}

uint32_t SECONDARY_MEMORY::WriteMem(uint32_t address, uint32_t data)
{
    if (address < this->size)
    {
        storage[getRow(address)][getCol(address)] = data;
        return data;
    }
    return MEMORY_ACCESS_ERROR;
}

uint32_t SECONDARY_MEMORY::DeleteData(uint32_t address)
{
    if (address < this->size)
    {
        size_t r = getRow(address), c = getCol(address);
        if (storage[r][c] != MEMORY_ACCESS_ERROR)
        {
            uint32_t deletedData = storage[r][c];
            storage[r][c] = MEMORY_ACCESS_ERROR;
            return deletedData;
        }
    }
    return MEMORY_ACCESS_ERROR;
}
