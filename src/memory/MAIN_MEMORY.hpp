#ifndef MAIN_MEMORY_HPP
#define MAIN_MEMORY_HPP

#include <cstdint>
#include <unordered_map>
#define MEMORY_ACESS_ERROR 0

using std::size_t;
using std::uint32_t;
using std::unordered_map;

class MAIN_MEMORY
{
private:
    size_t size;
    unordered_map<uint32_t, uint32_t> ram;
    bool notFull();
    bool isEmpty();

public:
    MAIN_MEMORY(size_t size);
    ~MAIN_MEMORY();
    uint32_t ReadMem(uint32_t adress);
    uint32_t WriteMem(uint32_t adress, uint32_t data);
    uint32_t DeleteData(uint32_t adress);
};

MAIN_MEMORY::MAIN_MEMORY(size_t size)
{
    this->size = size;
}

MAIN_MEMORY::~MAIN_MEMORY()
{
    this->ram.clear();
}

bool MAIN_MEMORY::isEmpty()
{
    return this->ram.empty();
}

bool MAIN_MEMORY::notFull()
{
    return this->ram.size() < this->size;
}

uint32_t MAIN_MEMORY::ReadMem(uint32_t adress)
{
    if (this->notFull())
    {
        if (this->ram.count(adress) > 0)
        {
            return ram.at(adress);
        }
    }
    return MEMORY_ACESS_ERROR;
}

uint32_t MAIN_MEMORY::WriteMem(uint32_t adress, uint32_t data)
{
    if (this->notFull())
    {
        this->ram[adress] = data;
        return this->ram[adress];
    }
    return MEMORY_ACESS_ERROR;
}

uint32_t MAIN_MEMORY::DeleteData(uint32_t adress)
{
    if (!this->isEmpty())
    {
        if (this->ram.count(adress) > 0)
        {
            uint32_t deletedData = ram.at(adress);
            this->ram.erase(adress);
            return deletedData;
        }
        return MEMORY_ACESS_ERROR;
    }
    return MEMORY_ACESS_ERROR;
}

#endif
