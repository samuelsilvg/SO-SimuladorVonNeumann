#include "cache.hpp"
#include "cachePolicy.hpp"

Cache::Cache() {
    this -> capacity = CACHE_CAPACITY;
    this -> cacheMap.reserve(CACHE_CAPACITY);
    this -> cache_misses = 0;
    this -> cache_hits = 0;
}

Cache::~Cache() {
    this -> cacheMap.clear();
}


size_t Cache::get(size_t address){
    
    if(cacheMap.count(address) > 0){
        CacheEntry &entry = cacheMap[address];
        entry.isValid = true;
        cache_hits += 1;
        return entry.data; // Cache hit
    }
    
    cache_misses += 1;
    return CACHE_MISS; // Cache miss
}


void Cache::put(size_t address, size_t data, int currentTimestamp){

    CachePolicy cachepolicy;

    CacheEntry entry;
    entry.data = data;
    entry.isValid = true;
    entry.isDirty = false;
    entry.timestamp = currentTimestamp;
    size_t cacheValids = 0;
    
    for(auto &c : cacheMap){
        if(c.second.isValid){
            cacheValids++;
        } else {
            cacheMap.erase(c.first);
        }
    }

    if(cacheValids == capacity){
        if(cachepolicy.erase(cacheMap)){
            cacheMap[address] = entry;
        }
    } else {
        cacheMap[address] = entry;
    }
}

void Cache::update(size_t address, size_t data){
    cacheMap[address].data = data;
    cacheMap[address].isDirty = true;
}

void Cache::invalidate(){
    for(auto &c : cacheMap){
        c.second.isValid = 0;
    }
}