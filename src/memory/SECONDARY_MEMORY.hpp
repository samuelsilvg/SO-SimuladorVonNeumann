#ifndef SECONDARY_MEMORY_HPP
#define SECONDARY_MEMORY_HPP

#include <cstdint>
#include <vector>
#include <cstddef>
#include <cmath>

#define MEMORY_ACCESS_ERROR UINT32_MAX
#define MAX_SECONDARY_MEMORY_SIZE 8192

using std::size_t;
using std::uint32_t;
using std::vector;

class SECONDARY_MEMORY
{
private:
    size_t size;
    size_t rowSize;
    vector<vector<uint32_t>> storage;
    bool notFull();
    bool isEmpty();
    inline size_t getRow(uint32_t address) const;
    inline size_t getCol(uint32_t address) const;

public:
    SECONDARY_MEMORY(size_t size);
    ~SECONDARY_MEMORY();
    uint32_t ReadMem(uint32_t address);
    uint32_t WriteMem(uint32_t address, uint32_t data);
    uint32_t DeleteData(uint32_t address);
};

#endif
