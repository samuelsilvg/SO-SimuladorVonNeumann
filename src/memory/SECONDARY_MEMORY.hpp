#ifndef SECOND_MEMORY_HPP
#define SECOND_MEMORY_HPP

#include <cstdint>
#include <unordered_map>
#define MEMORY_ACESS_ERROR 0

using std::uint32_t;
using std::size_t;
using std::unordered_map;

class SECONDARY_MEMORY
{

private:
    size_t size;
    unordered_map<uint32_t, uint32_t> disk;
    bool notFull();
    bool isEmpty();

public:
    SECONDARY_MEMORY(size_t size);
    ~SECONDARY_MEMORY();
    uint32_t ReadMem(uint32_t adress);
    uint32_t WriteMem(uint32_t adress, uint32_t data);
    uint32_t DeleteData(uint32_t adress);
};

SECONDARY_MEMORY::SECONDARY_MEMORY(size_t size)
{
    this->size = size;
}

SECONDARY_MEMORY::~SECONDARY_MEMORY()
{
    this->disk.clear();
}

bool SECONDARY_MEMORY::isEmpty()
{
    return this->disk.empty();
}

bool SECONDARY_MEMORY::notFull()
{
    return this->disk.size() < this->size;
}

uint32_t SECONDARY_MEMORY::ReadMem(uint32_t adress)
{
    if (!this->isEmpty())
    {
        if (this->disk.count(adress) > 0)
        {
            return disk.at(adress);
        }
    }
    return MEMORY_ACESS_ERROR;
}

uint32_t SECONDARY_MEMORY::WriteMem(uint32_t adress, uint32_t data)
{
    if (this->notFull())
    {
        this->disk[adress] = data;
        return this->disk.at(adress);
    }
    return MEMORY_ACESS_ERROR;
}

uint32_t SECONDARY_MEMORY::DeleteData(uint32_t adress)
{
    if (!this->isEmpty())
    {
        if (this->disk.count(adress) > 0)
        {
            uint32_t deletedData = disk.at(adress);
            this->disk.erase(adress);
            return deletedData;
        }
        return MEMORY_ACESS_ERROR;
    }
    return MEMORY_ACESS_ERROR;
}
#endif