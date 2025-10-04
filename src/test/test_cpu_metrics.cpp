/*
  test_cpu_metrics.cpp
  Teste simples para exercitar o pipeline e imprimir métricas do PCB.
*/
#include <iostream>
#include <vector>
#include <memory>
#include <cstdint>

#include "cpu/pcb_loader.hpp"
#include "cpu/PCB.hpp"
#include "cpu/CONTROL_UNIT.hpp"
#include "cpu/REGISTER_BANK.hpp"
#include "cpu/ULA.hpp"
#include "cpu/HASH_REGISTER.hpp"

// CORREÇÃO: Caminho dos includes de memória e I/O ajustado
#include "memory/MemoryManager.hpp"
#include "IO/IOManager.hpp" // Define a estrutura IORequest

// Sentinel de fim de programa (mesmo usado em CONTROL_UNIT.cpp)
static constexpr uint32_t END_SENTINEL = 0b11111100000000000000000000000000u;

// Helpers de montagem de instruções (formato simplificado compatível com o decodificador atual)
static uint32_t makeR(uint8_t rs, uint8_t rt, uint8_t rd, uint8_t funct /*6 bits*/) {
    return (0u << 26) | (static_cast<uint32_t>(rs) << 21) | (static_cast<uint32_t>(rt) << 16) |
           (static_cast<uint32_t>(rd) << 11) | (0u << 6) | (funct & 0x3Fu);
}
static uint32_t makeI(uint8_t opcode /*6 bits*/, uint8_t rs, uint8_t rt, uint16_t imm) {
    return (static_cast<uint32_t>(opcode & 0x3F) << 26) | (static_cast<uint32_t>(rs) << 21) |
           (static_cast<uint32_t>(rt) << 16) | imm;
}
static uint32_t makeJ(uint8_t opcode /*6 bits*/, uint32_t addr26) {
    return (static_cast<uint32_t>(opcode & 0x3F) << 26) | (addr26 & 0x03FFFFFFu);
}


int main() {
    // Carrega PCB do JSON
    PCB pcb{};
    if (!load_pcb_from_json("process1.json", pcb)) {
        std::cerr << "[WARN] Falha ao carregar process1.json. Usando valores padrão.\n";
        pcb.pid = 1; pcb.name = "fallback"; pcb.quantum = 50; pcb.priority = 0;
    }

    // Agora precisamos de um MemoryManager em vez de uma MainMemory
    MemoryManager memManager(1024, 8192);

    // Índices de registradores (exemplo, ajuste se necessário)
    // Usando o mapa para ser mais robusto
    auto& mapper = hw::getGlobalRegisterMapper();
    uint8_t r_zero = hw::RegisterMapper::indexFromBinary(mapper.getRegisterBinary("zero"));
    uint8_t r_t1 = hw::RegisterMapper::indexFromBinary(mapper.getRegisterBinary("t1"));
    uint8_t r_t2 = hw::RegisterMapper::indexFromBinary(mapper.getRegisterBinary("t2"));
    uint8_t r_t3 = hw::RegisterMapper::indexFromBinary(mapper.getRegisterBinary("t3"));
    uint8_t r_t4 = hw::RegisterMapper::indexFromBinary(mapper.getRegisterBinary("t4"));

    // Instruções
    uint32_t li_t1 = makeI(0x0E, r_zero, r_t1, 5);
    uint32_t li_t2 = makeI(0x0E, r_zero, r_t2, 7);
    uint32_t add_t3 = makeR(r_t1, r_t2, r_t3, 0x20);
    uint32_t sw_t3  = makeI(0x0D, r_zero, r_t3, 20);
    uint32_t lw_t4  = makeI(0x0C, r_zero, r_t4, 20);
    uint32_t print_t4 = makeI(0x10, r_zero, r_t4, 0);

    // Escreve na memória usando o MemoryManager
    memManager.write(0, li_t1, pcb);
    memManager.write(4, li_t2, pcb); // Endereços avançam de 4 em 4
    memManager.write(8, add_t3, pcb);
    memManager.write(12, sw_t3, pcb);
    memManager.write(16, lw_t4, pcb);
    memManager.write(20, print_t4, pcb);
    memManager.write(24, END_SENTINEL, pcb);

    // Zera o endereço alvo inicial
    memManager.write(28, 0, pcb); // Usando um endereço diferente para o dado

    std::vector<std::unique_ptr<IORequest>> ioRequests;
    bool printLock = false;

    // Executa núcleo
    Core(memManager, pcb, &ioRequests, printLock);

    // Métricas
    std::cout << "=== METRICAS PCB ===\n";
    std::cout << "pid: " << pcb.pid << " name: " << pcb.name << " state: " << (pcb.state == State::Finished ? "Finished" : "NotFinished") << "\n";
    std::cout << "pipeline_cycles:      " << pcb.pipeline_cycles.load() << "\n";
    std::cout << "stage_invocations:    " << pcb.stage_invocations.load() << "\n";
    std::cout << "mem_reads:            " << pcb.mem_reads.load() << "\n";
    std::cout << "mem_writes:           " << pcb.mem_writes.load() << "\n";
    std::cout << "primary_mem_accesses: " << pcb.primary_mem_accesses.load() << "\n";
    std::cout << "secondary_mem_accesses: " << pcb.secondary_mem_accesses.load() << "\n";
    std::cout << "mem_accesses_total:   " << pcb.mem_accesses_total.load() << "\n";
    std::cout << "memory_cycles (peso): " << pcb.memory_cycles.load() << "\n";

    if (!ioRequests.empty()) {
        std::cout << "=== IO REQUESTS ===\n";
        for (auto &req : ioRequests) {
            std::cout << "PRINT -> " << req->msg << " (pid=" << (req->process? req->process->pid : -1) << ")\n";
        }
    }

    return 0;
}