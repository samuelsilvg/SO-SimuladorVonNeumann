#include "cachePolicy.hpp"


CachePolicy :: CachePolicy() {
    
}

CachePolicy :: ~CachePolicy() {

}

void CachePolicy :: erase(std :: unordered_map<size_t, CacheEntry> &cacheMap) {

    int menorTempo = INT_MAX;
    size_t enderecoRemover = 0;
    for(const auto &x : cacheMap){
        if(x.second.timestamp < menorTempo){
            menorTempo = x.second.timestamp;
            enderecoRemover = x.first;
        }
    }

    cacheMap.erase(enderecoRemover);
}




