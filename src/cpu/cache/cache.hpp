#ifndef CACHE_HPP
#define CACHE_HPP
#define CACHE_CAPACITY 16
#define CACHE_MISS UINT32_MAX
#include <bits/stdc++.h>


struct CacheEntry{
    size_t data;
    bool isValid;
    bool isDirty;
    int timestamp;
};

class Cache {
private: 
    std::unordered_map<size_t, CacheEntry> cacheMap;
    size_t capacity;
    int cache_misses;
    int cache_hits;

public:
    Cache();
    ~Cache();
    size_t get(size_t address);
    void put(size_t address, size_t data, int currentTimestamp);
    void update(size_t address, size_t data);
    void invalidate();
};

#endif