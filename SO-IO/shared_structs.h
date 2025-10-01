#ifndef SHARED_STRUCTS_H
#define SHARED_STRUCTS_H

#include <string>
#include <chrono>

// =======================
// Estado dos processos
// =======================
enum class State {
    Ready,
    Executing,
    Blocked,
    Finished
};

// =======================
// PCB (Process Control Block)
// =======================
struct PCB {
    int id;                  // Identificador do processo
    State state;              // Estado atual
    int total_cycles;         // Quantidade total de ciclos necessários
    int cycles_executed;      // Ciclos já executados
};

// =======================
// Estrutura de requisição de I/O
// =======================
struct IORequest {
    PCB* process;                                // Processo solicitante
    std::string operation;                       // Tipo da operação (ex: read_input, write_result, write_output)
    std::string msg;                             // Mensagem ou dado associado
    std::chrono::milliseconds cost_cycles;       // Custo simulado em tempo (latência)
};

#endif // SHARED_STRUCTS_H
