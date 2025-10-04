#ifndef CACHE_HPP
#define CACHE_HPP

#include <cstdint>
#include <cstddef> 
#include <unordered_map>
#include <vector>
#include <queue> // Adicionado para FIFO

#define CACHE_CAPACITY 16
#define CACHE_MISS UINT32_MAX

struct CacheEntry {
    size_t data;
    bool isValid;
    bool isDirty;
};
class MemoryManager;

class Cache {
private:
    std::unordered_map<size_t, CacheEntry> cacheMap;
    std::queue<size_t> fifo_queue; // Fila para controlar a ordem de entrada (FIFO)
    size_t capacity;
    int cache_misses;
    int cache_hits;

public:
    Cache();
    ~Cache();
    int get_misses();
    int get_hits();
    size_t get(size_t address);
    // O método put agora precisa interagir com o MemoryManager para o write-back
    void put(size_t address, size_t data, MemoryManager* memManager);
    void update(size_t address, size_t data);
    void invalidate();
    std::vector<std::pair<size_t, size_t>> dirtyData(); // Mantido para possíveis outras lógicas
};

#endif