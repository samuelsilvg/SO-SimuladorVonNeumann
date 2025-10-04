#include "cachePolicy.hpp"

CachePolicy::CachePolicy() {}

CachePolicy::~CachePolicy() {}

// Implementação da política FIFO
size_t CachePolicy::getAddressToReplace(std::queue<size_t>& fifo_queue) {
    if (fifo_queue.empty()) {
        return -1; // Retorna -1 se não houver nada para remover
    }

    // Pega o primeiro endereço que entrou na fila
    size_t address_to_remove = fifo_queue.front();
    // Remove da fila
    fifo_queue.pop();

    return address_to_remove;
}