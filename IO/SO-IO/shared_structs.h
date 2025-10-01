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
    int id;
    State state;
    int total_cycles;
    int cycles_executed;
};

// =======================
// Estrutura de requisição de I/O
// =======================
struct IORequest {
    PCB* process;
    std::string operation;
    std::string msg;
    std::chrono::milliseconds cost_cycles;
};

#endif 