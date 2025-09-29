/*
  CONTROL_UNIT.cpp (Provisório - JP)

  O que este arquivo "faz":
  - Lê instruções da memória, decodifica quais registradores e imediatos usar,
    manda as operações para a ULA (ALU), faz acesso à memória (load/store)
    e gera pedidos de I/O (print). Tudo isso dividido em 5 etapas
    (pipeline): IF, ID, EX, MEM, WB.
  Estrutura geral (visão rápida):
  - Helpers:
      * binaryStringToUint(...)  -> transforma uma string de '0'/'1' em número.
      * signExtend16(...)        -> transforma um imediato de 16 bits em 32 bits
                                   preservando o sinal (two's complement).
  - Utilitários para extrair campos da instrução de 32 bits:
      * Get_immediate(...)            -> pega os 16 bits de imediato.
      * Pick_Code_Register_Load(...)  -> pega o campo rt (bits 11..15).
      * Get_destination_Register(...) -> pega rd (bits 16..20).
      * Get_target_Register(...)      -> pega rt (bits 11..15).
      * Get_source_Register(...)      -> pega rs (bits 6..10).
  - Identificação de instrução:
      * Identificacao_instrucao(...)  -> lê os 6 bits do opcode e tenta
        retornar uma string com o nome da instrução ("ADD", "LW", "J", ...).
        Observação: o mapeamento está simplificado; R-type com opcode 000000
        tenta usar o campo 'funct' para inferir ADD/SUB/MULT/DIV.
  - Estágios do pipeline (implementados como métodos):
      * Fetch(context)   -> busca a instrução na memória usando o PC e escreve em IR.
                           Também detecta um sentinel de fim de programa.
      * Decode(regs, d)  -> lê a IR, identifica o mnemonic e preenche os campos
                           em Instruction_Data (registradores, imediato, etc).
                           Faz sign-extend dos imediatos quando necessário.
      * Execute(...)     -> dispatcher que decide qual execução fazer:
                           - Execute_Aritmetic_Operation(...) para ADD/SUB/...
                           - Execute_Loop_Operation(...) para BEQ/J/BLT/...
                           - Execute_Operation(...) para PRINT / I/O
      * Memory_Acess(...)-> realiza LW, SW, LA, LI e leitura para PRINT de
                           endereços de memória.
      * Write_Back(...)  -> grava na memória em caso de SW (ou outros writes se adicionados).

  O que o código espera encontrar (dependências que eu fiz):
  - Tipos/objetos já definidos em outros arquivos: MainMemory, PCB, ioRequest,
    REGISTER_BANK, ALU/ULA, Map (mapa de código->nome do registrador), State.
  - Instruções na memória são palavras de 32 bits; algumas funções tratam
    campos como strings de '0'/'1' (por simplicidade do protótipo).
  - O fim do programa é detectado por uma palavra sentinel (valor binário
    começando com 111111... conforme usado no projeto).
  Como usar (rápido):
  - Este arquivo implementa os métodos declarados em CONTROL_UNIT.hpp.
  - Ele é chamado pelo Core(...) (no loop de pipeline) que coordena os cinco
    estágios e controla quando o processo deve parar ou ser bloqueado.

  Observação final: pode estar confuso, mas me responsabilizo, então qualquer coisa só me perguntar.
  Estrutura exemplo seguida do PCB:
    {
    "pid": 123,
    "name": "ProcessoA",
    "quantum": 50,
    "priority": 5,
    "mem_weights": {
        "primary": 1,
        "secondary": 10
    },
    "tasks": [
        { "type": "run_code", "start_addr": 0, "end_addr": 120 },
        { "type": "io", "device": "printer", "when": 60 }
    ]
    }

*/

// Implementação da leitura do PCB (alinhar com I/O)
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

bool load_pcb_from_json(const std::string &path, PCB &pcb) {
    std::ifstream f(path);
    if (!f.is_open()) return false;
    json j; f >> j;

    pcb.pid = j.value("pid", 0);
    pcb.name = j.value("name", std::string("unknown"));
    pcb.quantum = j.value("quantum", 50);
    pcb.priority = j.value("priority", 0);
    pcb.memWeights.primary = j["mem_weights"].value("primary", 1ULL);
    pcb.memWeights.secondary = j["mem_weights"].value("secondary", 10ULL);
    return true;
}

// PCB.hpp (apenas os campos que vamos adicionar)
#include <atomic>
#include <string>
#include <vector>
#include <cstdint>

struct MemWeights {
    uint64_t primary = 1;
    uint64_t secondary = 10;
};

struct PCB {
    int pid;
    std::string name;
    int quantum;
    int priority;
    // --- Contadores atômicos (thread-safe)
    std::atomic<uint64_t> primary_mem_accesses{0};
    std::atomic<uint64_t> secondary_mem_accesses{0};
    std::atomic<uint64_t> memory_cycles{0}; // soma (weights)
    std::atomic<uint64_t> mem_accesses_total{0}; // total de acessos
    // opcional: contador total de ciclos (mem + pipeline)
    std::atomic<uint64_t> extra_cycles{0};
    // pipeline_cycles: conta cada iteração do loop principal (um ciclo global)
    std::atomic<uint64_t> pipeline_cycles{0};
    // stage_invocations: conta cada vez que um estágio do pipeline é chamado (IF, ID, EX, MEM, WB)
    std::atomic<uint64_t> stage_invocations{0};
    // mem_reads: quantidade de leituras de memória (ReadMem)
    std::atomic<uint64_t> mem_reads{0};
    // mem_writes: quantidade de escritas de memória (WriteMem)
    std::atomic<uint64_t> mem_writes{0};

    // Pesos configuráveis (do JSON)
    MemWeights memWeights;
};


#include "CONTROL_UNIT.hpp"
#include "MainMemory.hpp"   // Incluir a definição concreta quando estiver pronto (ou seja, provisório)
#include "PCB.hpp" // PCB = Process Control Block (ou bloco de controle de processo).
#include "ioRequest.hpp" // Estrutura para requisições de I/O

#include <bitset>
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <sstream>

// === Helpers internos (análogos ao ConvertToDecimalValue original) ===
static uint32_t binaryStringToUint(const std::string &bin) { // Converte uma string binária "010101" para uint32_t
    uint32_t value = 0;
    for (char c : bin) {
        value <<= 1;
        if (c == '1') value |= 1u;
        else if (c != '0') throw std::invalid_argument("binaryStringToUint: char n\'ao bin\u00e1rio");
    }
    return value;
}
static int32_t signExtend16(uint16_t v) { // Sign-extend (transformar sinal com complemento de 2) de 16 bits para 32 bits (retorna int32_t)
    if (v & 0x8000)
        return (int32_t)(0xFFFF0000u | v);
    else
        return (int32_t)(v & 0x0000FFFFu);
}

// ---------------------- Helpers adicionais (bitwise, úteis) ----------------------

// Converte um número small (0..31) em string de 5 bits "01010" (compat com map.mp)
static std::string regIndexToBitString(uint32_t idx) {
    std::string s;
    for (int i = 4; i >= 0; --i) s.push_back(((idx >> i) & 1) ? '1' : '0');
    return s;
}

// Converte valor para string binária com width bits (debug)
static std::string toBinStr(uint32_t v, int width) {
    std::string s(width, '0');
    for (int i = 0; i < width; ++i)
        s[width - 1 - i] = ((v >> i) & 1) ? '1' : '0';
    return s;
}

// Sign-extend genérico (1 <= bits <= 31)
static int32_t sign_extend(uint32_t value, unsigned bits) {
    const uint32_t mask = (bits >= 32) ? 0xFFFFFFFFu : ((1u << bits) - 1u);
    value &= mask;
    const uint32_t sign = 1u << (bits - 1);
    if (value & sign) value |= ~mask;
    return static_cast<int32_t>(value);
}

// Helper de contagem de memória (PCB)
static inline void account_memory_access(MainMemory &ram, PCB &process, uint32_t addr, bool isWrite = false) {
    bool secondary = ram.IsSecondary(addr);
    process.mem_accesses_total.fetch_add(1);
    // Atualiza leituras ou escritas
    if (isWrite) process.mem_writes.fetch_add(1); else process.mem_reads.fetch_add(1);
    if (secondary) {
        process.secondary_mem_accesses.fetch_add(1);
        process.memory_cycles.fetch_add(process.memWeights.secondary); // custo de acesso secundário
    } else {
        process.primary_mem_accesses.fetch_add(1);
        process.memory_cycles.fetch_add(process.memWeights.primary);   // custo de acesso primário
    }
}

// --- Helpers adicionais de instrumentação (pipeline) ---
// account_pipeline_cycle: chamado uma vez por ciclo global do loop Core
static inline void account_pipeline_cycle(PCB &p) { p.pipeline_cycles.fetch_add(1); }
// account_stage: chamado a cada estágio executado (IF, ID, EX, MEM, WB)
static inline void account_stage(PCB &p) { p.stage_invocations.fetch_add(1); }

// === Implementações de utilitários de extração ===
// (as implementações abaixo usam bitwise)
string Control_Unit::Get_immediate(const uint32_t instruction) {
    // Retorna os 16 bits menos significativos (posições 16..31) como string de '0'/'1'
    uint16_t imm = static_cast<uint16_t>(instruction & 0xFFFFu);
    return std::bitset<16>(imm).to_string();
}
string Control_Unit::Pick_Code_Register_Load(const uint32_t instruction) {
    // Para instruções de load escolha o campo 'rt' (bits 11..15)
    uint32_t rt = (instruction >> 16) & 0x1Fu;
    return regIndexToBitString(rt);
}
string Control_Unit::Get_destination_Register(const uint32_t instruction) {
    // R-type: rd em bits 16..20
    uint32_t rd = (instruction >> 11) & 0x1Fu;
    return regIndexToBitString(rd);
}
string Control_Unit::Get_target_Register(const uint32_t instruction) {
    // rt em bits 11..15
    uint32_t rt = (instruction >> 16) & 0x1Fu;
    return regIndexToBitString(rt);
}
string Control_Unit::Get_source_Register(const uint32_t instruction) {
    // rs em bits 6..10
    uint32_t rs = (instruction >> 21) & 0x1Fu;
    return regIndexToBitString(rs);
}

// === Identifica o mnemonic a partir da palavra de 32 bits ===
string Control_Unit::Identificacao_instrucao(uint32_t instruction, REGISTER_BANK & /*registers*/) {
    // Converte instrucao para string de bits ( implementação usa opcode/funct por bitwise)
    uint32_t opcode = (instruction >> 26) & 0x3Fu;
    std::string opcode_bin = toBinStr(opcode, 6);

    // Compara com o mapa de mnemônicos (códigos definidos em instructionMap)
    // Observação: esse mapeamento está simplificado (ou seja, com certeza pode ter erro)(tratando opcode diretamente como mnemônico)
    for (const auto &p : instructionMap) {
        if (p.second == opcode_bin) {
            // Retorna o mnemônico em uppercase consistente com o restante do código
            std::string key = p.first;
            // mapa de nomes no código original: "la" -> "LA", etc.
            for (auto &c : key) c = toupper(c);
            return key;
        }
    }
    // Se não encontrou no mapa, pode ser um R-type com opcode 000000
    if (opcode == 0) {
        // Aqui faríamos parse do funct (bits 26..31) se a implementação original usar funct.
        uint32_t funct = instruction & 0x3Fu; // bits 5..0
        // Como não temos o mapa de functões, tentamos inferir por alguns casos usuais
        if (funct == 0x20) return "ADD"; // exemplo MIPS real (100000)
        if (funct == 0x22) return "SUB"; // 100010
        if (funct == 0x18) return "MULT"; // 011000
        if (funct == 0x1A) return "DIV"; // 011010
    }
    // Se chegar aqui, instrução desconhecida
    return "";
}

// === Estágios do Pipeline ===
// IF: busca a instrucao na memória usando PC e grava em IR
void Control_Unit::Fetch(ControlContext &context) {
    // Instrumentação: conta estágio IF
    account_stage(context.process);
    // Primeiro coloca o MAR com o PC
    context.registers.mar.write(context.registers.pc.value);
    // Lê a memória
    uint32_t instr = context.ram.ReadMem(context.registers.mar.read());
    // Escreve no registrador IR
    context.registers.ir.write(instr);
    account_memory_access(context.ram, context.process, context.registers.mar.read());
    // Checa sentinel de fim de programa (padrão usado no projeto)
    const uint32_t END_SENTINEL = 0b11111100000000000000000000000000u;
    if (instr == END_SENTINEL) {
        // Marca fim do programa e não incrementa PC
        context.endProgram = true;
        return;
    }
    // Incrementa PC (cada posição representa uma "palavra" de instrução na memória do simulador)
    context.registers.pc.write(context.registers.pc.value + 1);
}

// ID: decodifica a instrução que está em IR e preenche o Instruction_Data
void Control_Unit::Decode(REGISTER_BANK &registers, Instruction_Data &data) {
    uint32_t instruction = registers.ir.read();
    // Armazena a palavra bruta
    data.rawInstruction = instruction;
    // Identifica o mnemônico
    data.op = Identificacao_instrucao(instruction, registers);
    // Para instruções R-type (aritméticas) carregamos rs/rt/rd
    if (data.op == "ADD" || data.op == "SUB" || data.op == "MULT" || data.op == "DIV") {
        data.source_register = Get_source_Register(instruction);
        data.target_register = Get_target_Register(instruction);
        data.destination_register = Get_destination_Register(instruction);
    }
    // I-type com imediato / load / store / li / la
    else if (data.op == "LI" || data.op == "LW" || data.op == "LA" || data.op == "SW") {
        data.target_register = Get_target_Register(instruction);
        data.addressRAMResult = Get_immediate(instruction);
        // sign-extend imediato para uso posterior
        uint16_t imm16 = static_cast<uint16_t>(instruction & 0xFFFFu);
        data.immediate = signExtend16(imm16);
    }
    // Branches
    else if (data.op == "BLT" || data.op == "BGT" || data.op == "BGTI" || data.op == "BLTI" || data.op == "BEQ" || data.op == "BNE") {
        data.source_register = Get_source_Register(instruction);
        data.target_register = Get_target_Register(instruction);
        data.addressRAMResult = Get_immediate(instruction);
        uint16_t imm16 = static_cast<uint16_t>(instruction & 0xFFFFu);
        data.immediate = signExtend16(imm16);
    }
    // Jump
    else if (data.op == "J") {
        // extract lower 26 bits for J type (kept as string for compat)
        uint32_t instr26 = instruction & 0x03FFFFFFu;
        data.addressRAMResult = std::bitset<26>(instr26).to_string();
        data.immediate = static_cast<int32_t>(instr26);
    }
    // PRINT (pode imprimir registrador ou memória)
    else if (data.op == "PRINT") {
        data.target_register = Get_target_Register(instruction);
        std::string imm = Get_immediate(instruction);
        // Se imediato é zero (toda a cadeia zero), assumimos que é registrador
        bool allZero = true;
        for (char c : imm) if (c != '0') { allZero = false; break; }
        if (!allZero) {
            data.addressRAMResult = imm; // endereco direto
            uint16_t imm16 = static_cast<uint16_t>(binaryStringToUint(imm));
            data.immediate = signExtend16(imm16);
        } else {
            data.addressRAMResult.clear();
            data.immediate = 0;
        }
    }
    // Se não reconhecida, mantemos op vazia
}

// EX: operações que utilizam a ULA (ALU)
void Control_Unit::Execute_Aritmetic_Operation(REGISTER_BANK &registers, Instruction_Data &data) {
    std::string name_rs = this->map.mp[data.source_register];
    std::string name_rt = this->map.mp[data.target_register];
    std::string name_rd = this->map.mp[data.destination_register];

    ALU alu;
    if (data.op == "ADD") {
        alu.A = registers.acessoLeituraRegistradores[name_rs]();
        alu.B = registers.acessoLeituraRegistradores[name_rt]();
        alu.op = ADD;
        alu.calculate();
        registers.acessoEscritaRegistradores[name_rd](alu.result);
    } else if (data.op == "SUB") {
        alu.A = registers.acessoLeituraRegistradores[name_rs]();
        alu.B = registers.acessoLeituraRegistradores[name_rt]();
        alu.op = SUB;
        alu.calculate();
        registers.acessoEscritaRegistradores[name_rd](alu.result);
    } else if (data.op == "MULT") {
        alu.A = registers.acessoLeituraRegistradores[name_rs]();
        alu.B = registers.acessoLeituraRegistradores[name_rt]();
        alu.op = MUL;
        alu.calculate();
        registers.acessoEscritaRegistradores[name_rd](alu.result);
    } else if (data.op == "DIV") {
        alu.A = registers.acessoLeituraRegistradores[name_rs]();
        alu.B = registers.acessoLeituraRegistradores[name_rt]();
        alu.op = DIV;
        alu.calculate();
        registers.acessoEscritaRegistradores[name_rd](alu.result);
    }
}

// EX / SYSCALL / IO: aqui tratamos PRINT e operações que causam I/O
void Control_Unit::Execute_Operation(Instruction_Data &data, ControlContext &context) {
    // Instrumentação: conta estágio EX (operação específica PRINT/syscall)
    // (dispatcher já contou uma vez para EX; não duplicar aqui para manter métrica coesa)
    if (data.op == "PRINT") {
        // Se target_register está preenchido -> imprimir registrador
        if (!data.target_register.empty()) {
            std::string name = this->map.mp[data.target_register];
            int value = context.registers.acessoLeituraRegistradores[name]();
            auto req = std::make_unique<ioRequest>();
            req->msg = std::to_string(value);
            req->process = &context.process;
            context.ioRequests.push_back(std::move(req));

            if (context.printLock) {
                context.process.state = State::Blocked;
                context.endExecution = true;
            }
        }
        // Caso contrario, o print foi criado com um imediato (endereco) e 
        // o acesso é tratado em Memory_Acess para manter a semântica do pipeline.
    }
}

// EX: branches e jumps. Usamos a ALU para calcular condições
void Control_Unit::Execute_Loop_Operation(REGISTER_BANK &registers, Instruction_Data &data,
                                          int &counter, int &counterForEnd, bool &programEnd,
                                          MainMemory &ram, PCB &process) {
    std::string name_rs = this->map.mp[data.source_register];
    std::string name_rt = this->map.mp[data.target_register];

    ALU alu;

    if (data.op == "BEQ") {
        alu.A = registers.acessoLeituraRegistradores[name_rs]();
        alu.B = registers.acessoLeituraRegistradores[name_rt]();
        alu.op = BEQ;
        alu.calculate();
        if (alu.result == 1) {
            uint32_t addr = binaryStringToUint(data.addressRAMResult);
            registers.pc.write(addr);
            registers.ir.write(ram.ReadMem(registers.pc.read()));
            counter = 0; counterForEnd = 5; programEnd = false;
            
        }
    } else if (data.op == "BNE") {
        alu.A = registers.acessoLeituraRegistradores[name_rs]();
        alu.B = registers.acessoLeituraRegistradores[name_rt]();
        alu.op = BNE;
        alu.calculate();
        if (alu.result == 1) {
            uint32_t addr = binaryStringToUint(data.addressRAMResult);
            registers.pc.write(addr);
            registers.ir.write(ram.ReadMem(registers.pc.read()));
            counter = 0; counterForEnd = 5; programEnd = false;
        }
    } else if (data.op == "J") {
        uint32_t addr = binaryStringToUint(data.addressRAMResult);
        registers.pc.write(addr);
        registers.ir.write(ram.ReadMem(registers.pc.read()));
        counter = 0; counterForEnd = 5; programEnd = false;
    } else if (data.op == "BLT") {
        alu.A = registers.acessoLeituraRegistradores[name_rs]();
        alu.B = registers.acessoLeituraRegistradores[name_rt]();
        alu.op = BLT;
        alu.calculate();
        if (alu.result == 1) {
            uint32_t addr = binaryStringToUint(data.addressRAMResult);
            registers.pc.write(addr);
            registers.ir.write(ram.ReadMem(registers.pc.read()));
            counter = 0; counterForEnd = 5; programEnd = false;
        }
    } else if (data.op == "BGT") {
        alu.A = registers.acessoLeituraRegistradores[name_rs]();
        alu.B = registers.acessoLeituraRegistradores[name_rt]();
        alu.op = BGT;
        alu.calculate();
        if (alu.result == 1) {
            uint32_t addr = binaryStringToUint(data.addressRAMResult);
            registers.pc.write(addr);
            registers.ir.write(ram.ReadMem(registers.pc.read()));
            counter = 0; counterForEnd = 5; programEnd = false;
        }
    }
    // Observação: contabilização de acesso após mudança de PC (pré-decode da próxima instrução)
    // O acesso de memória real do próximo ciclo será no Fetch, mas se a lógica de branch aqui realizar
    // um ReadMem antecipado (já feito no write do IR) contabilizamos este acesso:
    // Conta o acesso antecipado ao carregar a próxima instrução (já fizemos ReadMem ao atualizar IR)
    account_memory_access(ram, process, registers.pc.read());
}

// Dispatcher: decide qual caminho seguir para a instrução
void Control_Unit::Execute(Instruction_Data &data, ControlContext &context) {
    // Instrumentação: conta estágio EX (apenas uma vez por dispatcher)
    account_stage(context.process);
    if (data.op == "ADD" || data.op == "SUB" || data.op == "MULT" || data.op == "DIV") {
        Execute_Aritmetic_Operation(context.registers, data);
    } else if (data.op == "BEQ" || data.op == "J" || data.op == "BNE" || data.op == "BGT" || data.op == "BGTI" || data.op == "BLT" || data.op == "BLTI") {
    Execute_Loop_Operation(context.registers, data, context.counter, context.counterForEnd, context.endProgram, context.ram, context.process);
    } else if (data.op == "PRINT") {
        Execute_Operation(data, context);
    }
}

 // MEM: acessos à memória (load/store) e operações relacionadas
void Control_Unit::Memory_Acess(Instruction_Data &data, ControlContext &context) {
    // Instrumentação: conta estágio MEM
    account_stage(context.process);
    // Se target_register estiver vazio e for PRINT, tratamos leitura de memória para impressão
    std::string name_rt = this->map.mp[data.target_register];
    if (data.op == "LW") {
        uint32_t addr = binaryStringToUint(data.addressRAMResult);
        int value = context.ram.ReadMem(addr);
        account_memory_access(context.ram, context.process, addr, false);
        // Escreve no registrador destino (rt)
        context.registers.acessoEscritaRegistradores[name_rt](value);
    } else if (data.op == "LA" || data.op == "LI") {
        // LA/LI colocam o imediato (endereco ou valor) direto no registrador
        uint32_t val = binaryStringToUint(data.addressRAMResult);
        context.registers.acessoEscritaRegistradores[name_rt](static_cast<int>(val));
    } else if (data.op == "PRINT" && data.target_register.empty()) {
        uint32_t addr = binaryStringToUint(data.addressRAMResult);
        int value = context.ram.ReadMem(addr);
        account_memory_access(context.ram, context.process, addr, false);
        auto req = std::make_unique<ioRequest>();
        req->msg = std::to_string(value);
        req->process = &context.process;
        context.ioRequests.push_back(std::move(req));
        if (context.printLock) {
            context.process.state = State::Blocked;
            context.endExecution = true;
        }
    }
}

// WB: grava resultados que eventualmente precisam voltar para a memória (no projeto original, SW 
// é tratado aqui como write back para memória)
void Control_Unit::Write_Back(Instruction_Data &data, ControlContext &context) {
    // Instrumentação: conta estágio WB
    account_stage(context.process);
    if (data.op == "SW") {
        uint32_t addr = binaryStringToUint(data.addressRAMResult);
        std::string name_rt = this->map.mp[data.target_register];
        int value = context.registers.acessoLeituraRegistradores[name_rt]();
        context.ram.WriteMem(addr, value);
        account_memory_access(context.ram, context.process, addr, true);
    }
}

// ------------------------------
// Loop principal do pipeline
// ------------------------------
// Esta função é a mesma assinatura declarada no header CONTROL_UNIT.hpp.
// Ela cria a unidade de controle, monta o contexto e executa o loop
// que simula o pipeline de 5 estágios usando os contadores.
//
// A ideia do loop:
//  - `counter` é um ponteiro lógico que sobe a cada ciclo e determina quais
//    estágios serão chamados (IF, ID, EX, MEM, WB) sobre instruções
//    que já entraram no pipeline.
//  - `counterForEnd` controla quantos ciclos ainda faltam para esvaziar
//    o pipeline quando decidimos parar (drain).
//  - `clock` conta quantos ciclos o processo já rodou (usado com quantum).
//  - `endExecution` é setado quando o quantum acaba ou quando achamos
//    a instrução sentinel de fim de programa; então, começamos a decrementar
//    `counterForEnd` até 0 para garantir que todas instruções em voo terminem.
//  - `UC.data` é um buffer simples (FIFO implícito) onde empilhamos
//    `Instruction_Data` para acompanhar qual informação cada estágio deve usar.

void* Core(MainMemory &ram, PCB &process, vector<unique_ptr<ioRequest>>* ioRequests, bool &printLock) {
    // pega referência direta ao banco de registradores do processo
    auto &registers = process.regBank;

    // cria unidade de controle e um template de Instruction_Data
    Control_Unit UC;
    Instruction_Data data; // entrada vazia que será empurrada no pipeline

    // contadores e flags (iniciais)
    int clock = 0;             // ciclos já executados pelo processo (para quantum)
    int counterForEnd = 5;     // quantos ciclos faltam para esvaziar o pipeline
    int counter = 0;           // avança a cada ciclo; usado para indexar estágios
    bool endProgram = false;   // sinaliza que encontramos a instrução de fim
    bool endExecution = false; // sinaliza para iniciar o esvaziamento do pipeline

    // monta o contexto que será passado a todos os métodos
    ControlContext context{ registers, ram, *ioRequests, printLock, process, counter, counterForEnd, endProgram, endExecution };

    // Enquanto houver instruções em voo no pipeline (counterForEnd > 0)
    while (context.counterForEnd > 0) {

        // --- WRITE BACK (estágio 5)
        // Só chamamos se já houver instrução suficiente no buffer
        // atenção ao índice: context.counter - 4
        if (context.counter >= 4 && context.counterForEnd >= 1) {
            UC.Write_Back(UC.data[context.counter - 4], context);
        }

        // --- MEMORY ACCESS (estágio 4)
        if (context.counter >= 3 && context.counterForEnd >= 2) {
            UC.Memory_Acess(UC.data[context.counter - 3], context);
        }

        // --- EXECUTE (estágio 3)
        if (context.counter >= 2 && context.counterForEnd >= 3) {
            UC.Execute(UC.data[context.counter - 2], context);
        }

        // --- DECODE (estágio 2)
        if (context.counter >= 1 && context.counterForEnd >= 4) {
            // Instrumentação: conta estágio ID
            account_stage(process);
            // Decode lê o IR atual do registro e preenche o Instruction_Data
            UC.Decode(context.registers, UC.data[context.counter - 1]);
        }

        // --- FETCH (estágio 1)
        // Só fazemos fetch de uma nova instrução enquanto estivermos aceitando
        // novas instruções (counterForEnd == 5 significa que pipeline está aberto)
        if (context.counter >= 0 && context.counterForEnd == 5) {
            // Instrumentação: conta estágio IF (Fetch) já é feita dentro de Fetch
            // empurra um slot vazio no buffer e faz o fetch que grava IR no registrador
            UC.data.push_back(data);
            UC.Fetch(context);
            // Observe: Fetch escreve a IR; o Decode do próximo ciclo irá ler a IR
        }

        // avança o tempo do pipeline
        context.counter += 1;
        clock += 1;
    // Instrumentação: contabiliza um ciclo global do pipeline
    account_pipeline_cycle(process);

        // se atingimos o quantum do processo, ou achamos END, sinalizar para terminar
        if (clock >= process.quantum || context.endProgram == true) {
            context.endExecution = true;
        }

        // quando endExecution estiver true começamos a decrementar counterForEnd
        // (drain do pipeline). Isso garante que instruções já fetchadas terminem.
        if (context.endExecution == true) {
            context.counterForEnd -= 1;
        }
    }

    // se o fim do programa foi detectado, marcamos o processo como finalizado
    if (context.endProgram) {
        context.process.state = State::Finished;
    }

    // retorno nulo (compatível com sua assinatura); se quiser, pode retornar
    // um status ou ponteiro mais informativo.
    return nullptr;
}