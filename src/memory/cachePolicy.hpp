#ifndef CACHE_POLICY_HPP
#define CACHE_POLICY_HPP

#include "cache.hpp"
#include <cstddef>
#include <queue> // Incluir para std::queue

class CachePolicy {
public:
    CachePolicy();
    ~CachePolicy();

    // Retorna o endereço a ser substituído com base na política FIFO
    size_t getAddressToReplace(std::queue<size_t>& fifo_queue);
};

#endif