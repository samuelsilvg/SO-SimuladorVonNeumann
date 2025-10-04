#ifndef PCB_HPP
#define PCB_HPP
/*
  PCB.hpp
  Definição do bloco de controle de processo (PCB) usado pelo simulador da CPU.
  Centraliza: identificação do processo, prioridade, quantum, pesos de memória e
  contadores de instrumentação de pipeline/memória.
*/
#include <string>
#include <atomic>
#include <cstdint>

#include "REGISTER_BANK.hpp" // necessidade de objeto completo dentro do PCB


// Estados possíveis do processo (simplificado)
enum class State {
    Ready,
    Running,
    Blocked,
    Finished
};

struct MemWeights {
    uint64_t cache = 1;   // custo por acesso à memória cache
    uint64_t primary = 5; // custo por acesso à memória primária
    uint64_t secondary = 10; // custo por acesso à memória secundária
};

struct PCB {
    int pid = 0;
    std::string name;
    int quantum = 0;
    int priority = 0;

    // Estado de execução
    State state = State::Ready;

    // Banco de registradores associado (definido fora, agregado aqui)
    hw::REGISTER_BANK regBank;

    // Contadores de acesso à memória
    std::atomic<uint64_t> primary_mem_accesses{0};
    std::atomic<uint64_t> secondary_mem_accesses{0};
    std::atomic<uint64_t> memory_cycles{0};      // soma ponderada pelos pesos
    std::atomic<uint64_t> mem_accesses_total{0}; // total bruto de acessos
    std::atomic<uint64_t> extra_cycles{0};       // reservado para extensões
    std::atomic<uint64_t> cache_mem_accesses{0}; // total de acessos à memória cache

    // Instrumentação detalhada
    std::atomic<uint64_t> pipeline_cycles{0};
    std::atomic<uint64_t> stage_invocations{0};
    std::atomic<uint64_t> mem_reads{0};
    std::atomic<uint64_t> mem_writes{0};

    MemWeights memWeights; // pesos configuráveis
};

#endif // PCB_HPP