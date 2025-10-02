#ifndef CACHE_POLICY_HPP
#define CACHE_POLICY_HPP
#include "cache.hpp"

#include <bits/stdc++.h>

class CachePolicy {

public:
    CachePolicy();
    ~CachePolicy();

    void erase(std::unordered_map<size_t, CacheEntry> &cacheMap);

};

#endif