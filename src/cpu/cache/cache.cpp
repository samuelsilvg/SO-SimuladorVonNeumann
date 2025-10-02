#include "cache.hpp"
#include "cachePolicy.hpp"

Cache::Cache() {
    this -> capacity = CACHE_CAPACITY;
    this -> currentTime = 0;
    this -> cacheMap.reserve(CACHE_CAPACITY);
}

Cache::~Cache() {
    this -> cacheMap.clear();
}


size_t Cache::get(size_t address){
    
    if(cacheMap.count(address) > 0){
        CacheEntry &entry = cacheMap[address];
        if(entry.isValid){
            return entry.data; // Cache hit
        }
    }

    return CACHE_MISS; // Cache miss
}


void Cache::put(size_t address, size_t data, int currentTimestamp){

    CachePolicy cachepolicy;

    CacheEntry entry;
    entry.data = data;
    entry.isValid = false;
    entry.isDirty = false;
    entry.timestamp = currentTimestamp;

    if(cacheMap.size() >= capacity){
        cachepolicy.erase(cacheMap);
    }

    cacheMap[address] = entry;

}