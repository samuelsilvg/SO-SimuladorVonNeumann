#ifndef MAIN_MEMORY_HPP
#define MAIN_MEMORY_HPP

#include <cstdint>
#include <vector>

#define MEMORY_ACCESS_ERROR UINT32_MAX
#define MAX_MEMORY_SIZE 1024

using std::size_t;
using std::uint32_t;
using std::vector;

class MAIN_MEMORY
{
private:
    size_t size;
    vector<uint32_t> ram;
    bool notFull();
    bool isEmpty();

public:
    MAIN_MEMORY(size_t size);
    ~MAIN_MEMORY();
    uint32_t ReadMem(uint32_t address);
    uint32_t WriteMem(uint32_t address, uint32_t data);
    uint32_t DeleteData(uint32_t address);
};

#endif