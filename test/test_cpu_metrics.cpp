/*
  test_cpu_metrics.cpp
  Teste simples para exercitar o pipeline e imprimir métricas do PCB.
  Requisitos:
    - process1.json presente na raiz do workspace (carregado via load_pcb_from_json)
    - MainMemory deve expor WriteMem(uint32_t addr, uint32_t value) e ReadMem
    - Core(MainMemory&, PCB&, ...) executa o pipeline
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
#include "cpu/pcb_loader.hpp"

#include "MainMemory.hpp"
#include "ioRequest.hpp"

// Sentinel de fim de programa (mesmo usado em CONTROL_UNIT.cpp)
static constexpr uint32_t END_SENTINEL = 0b11111100000000000000000000000000u;

// Helpers de montagem de instruções (formato simplificado compatível com o decodificador atual)
// R-type (opcode 000000), funct diferencia ADD/SUB/MULT/DIV.
static uint32_t makeR(uint8_t rs, uint8_t rt, uint8_t rd, uint8_t funct /*6 bits*/) {
    return (0u << 26) | (static_cast<uint32_t>(rs) << 21) | (static_cast<uint32_t>(rt) << 16) |
           (static_cast<uint32_t>(rd) << 11) | (0u << 6) | (funct & 0x3Fu);
}
// I-type com opcode explícito (6 bits)
static uint32_t makeI(uint8_t opcode /*6 bits*/, uint8_t rs, uint8_t rt, uint16_t imm) {
    return (static_cast<uint32_t>(opcode & 0x3F) << 26) | (static_cast<uint32_t>(rs) << 21) |
           (static_cast<uint32_t>(rt) << 16) | imm;
}
// J-type: opcode + 26 bits de endereço (já alinhado na memória simulada)
static uint32_t makeJ(uint8_t opcode /*6 bits*/, uint32_t addr26) {
    return (static_cast<uint32_t>(opcode & 0x3F) << 26) | (addr26 & 0x03FFFFFFu);
}

// Opcodes conforme instructionMap (minúsculo no mapa, mas aqui usamos valor binário):
// add -> 000000 (R-type + funct 0x20)
// lw  -> 001100 (0x0C)
// sw  -> 001101 (0x0D)
// li  -> 001110 (0x0E)
// la  -> 001111 (0x0F)
// print -> 010000 (0x10)
// j   -> 001011 (0x0B)
// ATENÇÃO: Branches no mapa (ex: beq 000101) não são usados no teste básico abaixo.

int main() {
    // Carrega PCB do JSON
    PCB pcb{};
    if (!load_pcb_from_json("process1.json", pcb)) {
        std::cerr << "[WARN] Falha ao carregar process1.json. Usando valores padrão.\n";
        pcb.pid = 1; pcb.name = "fallback"; pcb.quantum = 50; pcb.priority = 0;
    }

    // Configuração inicial opcional de registradores (se necessário)
    // Exemplo: pcb.regBank.acessoEscritaRegistradores["t1"](10);

    MainMemory ram; // Assumindo construtor default

    // Programa de teste na memória:
    // 0: LI t1, 5        (carrega imediato 5 em t1)  opcode li
    // 1: LI t2, 7        (carrega 7 em t2)
    // 2: ADD t3 = t1 + t2 (funct 0x20)
    // 3: SW t3 -> [20]   (guarda resultado em endereço 20)
    // 4: LW t4 <- [20]   (carrega de volta)
    // 5: PRINT t4        (imprime registrador t4)
    // 6: END sentinel
    // Observação: O formato real dos registradores depende do seu map (HASH_REGISTER). Ajuste índices se necessário.

    // Escolha de índices de registradores (rs/rt/rd). Aqui usamos convenção simples:
    uint8_t r_zero = 0; // se existir
    uint8_t r_t1 = 1;
    uint8_t r_t2 = 2;
    uint8_t r_t3 = 3;
    uint8_t r_t4 = 4;

    // LI: opcode 001110 (0x0E) -> usamos campo rt (target) e imediato.
    uint32_t li_t1 = makeI(0x0E, r_zero, r_t1, 5);   // t1 = 5
    uint32_t li_t2 = makeI(0x0E, r_zero, r_t2, 7);   // t2 = 7
    uint32_t add_t3 = makeR(r_t1, r_t2, r_t3, 0x20); // ADD t3 = t1 + t2
    uint32_t sw_t3  = makeI(0x0D, r_zero, r_t3, 20); // SW t3 -> [20]
    uint32_t lw_t4  = makeI(0x0C, r_zero, r_t4, 20); // LW t4 <- [20]
    uint32_t print_t4 = makeI(0x10, r_zero, r_t4, 0); // PRINT t4 (imediato zero => registrador)

    ram.WriteMem(0, li_t1);
    ram.WriteMem(1, li_t2);
    ram.WriteMem(2, add_t3);
    ram.WriteMem(3, sw_t3);
    ram.WriteMem(4, lw_t4);
    ram.WriteMem(5, print_t4);
    ram.WriteMem(6, END_SENTINEL);

    // Para validar load/store
    ram.WriteMem(20, 0); // endereço alvo inicial

    std::vector<std::unique_ptr<ioRequest>> ioRequests;
    bool printLock = false; // se true, PRINT bloqueia processo

    // Executa núcleo
    Core(ram, pcb, &ioRequests, printLock);

    // Exibe resultados básicos de registradores se seu REGISTER_BANK expuser interface (exemplo fictício):
    // if (pcb.regBank.acessoLeituraRegistradores.contains("t4")) {
    //     std::cout << "t4 = " << pcb.regBank.acessoLeituraRegistradores["t4"]() << "\n";
    // }

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

    // I/O gerado
    if (!ioRequests.empty()) {
        std::cout << "=== IO REQUESTS ===\n";
        for (auto &req : ioRequests) {
            std::cout << "PRINT -> " << req->msg << " (pid=" << (req->process? req->process->pid : -1) << ")\n";
        }
    }

    return 0;
}
