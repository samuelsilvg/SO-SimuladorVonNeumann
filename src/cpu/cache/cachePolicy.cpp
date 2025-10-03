#include "cachePolicy.hpp"


CachePolicy :: CachePolicy() {
    
}

CachePolicy :: ~CachePolicy() {

}

bool CachePolicy :: erase(std :: unordered_map<size_t, CacheEntry> &cacheMap) {

    int menorTempo = INT_MAX;
    size_t enderecoRemover = SIZE_MAX;
    for(const auto &x : cacheMap){
        if(x.second.timestamp < menorTempo && !x.second.isDirty){
            menorTempo = x.second.timestamp;
            enderecoRemover = x.first;
        }
    }
    
    if(enderecoRemover != SIZE_MAX){
        cacheMap.erase(enderecoRemover);
        return true;
    } else{
        return false;
    }
}