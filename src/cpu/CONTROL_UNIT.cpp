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
      * Get_destination_Register(...) -> pega rd (bits 16..20).
      * Get_target_Register(...)      -> pega rt (bits 11..15).
      * Get_source_Register(...)      -> pega rs (bits 6..10).
  - Identificação de instrução:
      * Identificacao_instrucao(...)  -> lê os 6 bits do opcode e tenta
        retornar uma string com o nome da instrução (\"ADD\", \"LW\", \"J\", ...).
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
  Próximos passos sugeridos :
  - Tornar o mapeamento opcode/funct completo para suportar todos os R-types.
  - Substituir manipulação por strings binárias por operações bitwise diretas.
  - Implementar simples forwarding / detecção de hazards (se quiser modelar
    dependências entre instruções).
  - Adicionar logs ou modos de debug para visualizar cada estágio.

  Observação final: pode estar confuso, mas me responsabilizo, então qualquer coisa só me perguntar.
*/


#include "CONTROL_UNIT.hpp"
#include "MainMemory.hpp"   // Incluir a definição concreta quando estiver pronto (ou seja, provisório)
#include "PCB.hpp" // PCB = Process Control Block (ou bloco de controle de processo).
#include "ioRequest.hpp" // Estrutura para requisições de I/O

#include <bitset>
#include <cmath>
#include <stdexcept>
#include <iostream>

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
// === Implementações de utilitários de extração ===
string Control_Unit::Get_immediate(uint32_t instruction) {
    // Retorna os 16 bits menos significativos (posições 16..31) como string de '0'/'1'
    std::string bits = std::bitset<32>(instruction).to_string();
    return bits.substr(16, 16);
}
string Control_Unit::Get_destination_Register(uint32_t instruction) {
    // R-type: rd em bits 16..20
    std::string bits = std::bitset<32>(instruction).to_string();
    return bits.substr(16, 5);
}
string Control_Unit::Get_target_Register(uint32_t instruction) {
    // rt em bits 11..15
    std::string bits = std::bitset<32>(instruction).to_string();
    return bits.substr(11, 5);
}
string Control_Unit::Get_source_Register(uint32_t instruction) {
    // rs em bits 6..10
    std::string bits = std::bitset<32>(instruction).to_string();
    return bits.substr(6, 5);
}
// === Identifica o mnemonic a partir da palavra de 32 bits ===
string Control_Unit::Identificacao_instrucao(uint32_t instruction, REGISTER_BANK & /*registers*/) const {
    // Converte instrucao para string de bits
    std::string instr_bits = std::bitset<32>(instruction).to_string();
    std::string opcode = instr_bits.substr(0, 6);
    // Compara com o mapa de mnemônicos (códigos definidos em instructionMap)
    // Observação: esse mapeamento é simplificado (ou seja, com certeza pode ter erro)(tratando opcode diretamente como mnemônico)
    for (const auto &p : instructionMap) {
        if (p.second == opcode) {
            // Retorna o mnemônico em uppercase consistente com o restante do código
            std::string key = p.first;
            // mapa de nomes no código original: "la" -> "LA", etc.
            for (auto &c : key) c = toupper(c);
            return key;
        }
    }
    // Se não encontrou no mapa, pode ser um R-type com opcode 000000
    if (opcode == "000000") {
        // Aqui faríamos parse do funct (bits 26..31) se a implementação original usar funct.
        std::string funct = instr_bits.substr(26, 6);
        // Como não temos o mapa de functões, tentamos inferir por alguns casos usuais
        if (funct == "100000") return "ADD"; // exemplo MIPS real
        if (funct == "100010") return "SUB";
        if (funct == "011000") return "MULT";
        if (funct == "011010") return "DIV";
        if (funct == "100100") return "AND"; // AND lógico bit a bit (funct MIPS clássico)
    }
    // Se chegar aqui, instrução desconhecida
    return "";
}
// === Estágios do Pipeline ===
// IF: busca a instrucao na memória usando PC e grava em IR
void Control_Unit::Fetch(ControlContext &context) {
    // Primeiro coloca o MAR com o PC
    context.registers.mar.write(context.registers.pc.value);
    // Lê a memória
    uint32_t instr = context.ram.ReadMem(context.registers.mar.read());
    // Escreve no registrador IR
    context.registers.ir.write(instr);
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
    if (data.op == "ADD" || data.op == "SUB" || data.op == "MULT" || data.op == "DIV" || data.op == "AND") {
        data.source_register = Get_source_Register(instruction);
        data.target_register = Get_target_Register(instruction);
        data.destination_register = Get_destination_Register(instruction);
    }
    // I-type com imediato / load / store / li / la
    else if (data.op == "LI" || data.op == "LW" || data.op == "LA" || data.op == "SW") {
        data.target_register = Get_target_Register(instruction);
        data.addressRAMResult = Get_immediate(instruction);
        // sign-extend imediato para uso posterior
        uint16_t imm16 = static_cast<uint16_t>(binaryStringToUint(data.addressRAMResult));
        data.immediate = signExtend16(imm16);
    }
    // Branches
    else if (data.op == "BLT" || data.op == "BGT" || data.op == "BGTI" || data.op == "BLTI" || data.op == "BEQ" || data.op == "BNE") {
        data.source_register = Get_source_Register(instruction);
        data.target_register = Get_target_Register(instruction);
        data.addressRAMResult = Get_immediate(instruction);
        uint16_t imm16 = static_cast<uint16_t>(binaryStringToUint(data.addressRAMResult));
        data.immediate = signExtend16(imm16);
    }
    // Jump
    else if (data.op == "J") {
        data.addressRAMResult = Get_immediate(instruction);
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
    } else if (data.op == "AND") {
        alu.A = registers.acessoLeituraRegistradores[name_rs]();
        alu.B = registers.acessoLeituraRegistradores[name_rt]();
        alu.op = AND_OP;
        alu.calculate();
        registers.acessoEscritaRegistradores[name_rd](alu.result);
    }
}
// EX / SYSCALL / IO: aqui tratamos PRINT e operações que causam I/O
void Control_Unit::Execute_Operation(Instruction_Data &data, ControlContext &context) {
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
void Control_Unit::Execute_Loop_Operation(REGISTER_BANK &registers, Instruction_Data &data, int &counter, int &counterForEnd, bool &programEnd, MainMemory &ram) {
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
    } else if (data.op == "BGTI") {
        alu.A = registers.acessoLeituraRegistradores[name_rs]();
        alu.B = static_cast<uint32_t>(data.immediate);
        alu.op = BGTI;
        alu.calculate();
        if (alu.result == 1) {
            uint32_t addr = binaryStringToUint(data.addressRAMResult);
            registers.pc.write(addr);
            registers.ir.write(ram.ReadMem(registers.pc.read()));
            counter = 0; counterForEnd = 5; programEnd = false;
        }
    } else if (data.op == "BLTI") {
        alu.A = registers.acessoLeituraRegistradores[name_rs]();
        alu.B = static_cast<uint32_t>(data.immediate);
        alu.op = BLTI;
        alu.calculate();
        if (alu.result == 1) {
            uint32_t addr = binaryStringToUint(data.addressRAMResult);
            registers.pc.write(addr);
            registers.ir.write(ram.ReadMem(registers.pc.read()));
            counter = 0; counterForEnd = 5; programEnd = false;
        }
    }
}
// Dispatcher: decide qual caminho seguir para a instrução
void Control_Unit::Execute(Instruction_Data &data, ControlContext &context) {
    if (data.op == "ADD" || data.op == "SUB" || data.op == "MULT" || data.op == "DIV") {
        Execute_Aritmetic_Operation(context.registers, data);
    } else if (data.op == "BEQ" || data.op == "J" || data.op == "BNE" || data.op == "BGT" || data.op == "BGTI" || data.op == "BLT" || data.op == "BLTI") {
        Execute_Loop_Operation(context.registers, data, context.counter, context.counterForEnd, context.endProgram, context.ram);
    } else if (data.op == "PRINT") {
        Execute_Operation(data, context);
    }
}
 // MEM: acessos à memória (load/store) e operações relacionadas
void Control_Unit::Memory_Acess(Instruction_Data &data, ControlContext &context) {
    // Se target_register estiver vazio e for PRINT, tratamos leitura de memória para impressão
    std::string name_rt = this->map.mp[data.target_register];
    if (data.op == "LW") {
        uint32_t addr = binaryStringToUint(data.addressRAMResult);
        int value = context.ram.ReadMem(addr);
        // Escreve no registrador destino (rt)
        context.registers.acessoEscritaRegistradores[name_rt](value);
    } else if (data.op == "LA" || data.op == "LI") {
        // LA/LI colocam o imediato (endereco ou valor) direto no registrador
        uint32_t val = binaryStringToUint(data.addressRAMResult);
        context.registers.acessoEscritaRegistradores[name_rt](static_cast<int>(val));
    } else if (data.op == "PRINT" && data.target_register.empty()) {
        uint32_t addr = binaryStringToUint(data.addressRAMResult);
        int value = context.ram.ReadMem(addr);
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
    if (data.op == "SW") {
        uint32_t addr = binaryStringToUint(data.addressRAMResult);
        std::string name_rt = this->map.mp[data.target_register];
        int value = context.registers.acessoLeituraRegistradores[name_rt]();
        context.ram.WriteMem(addr, value);
    }
}

// ==============================
// Loop principal do pipeline
// ==============================
// Esta função é a mesma assinatura declarada no header CONTROL_UNIT.hpp.
// Ela cria a unidade de controle, monta o contexto e executa o loop
// que simula o pipeline de 5 estágios usando os contadores.
//
// A ideia do loop:
//  - "counter" é um ponteiro lógico que sobe a cada ciclo e determina quais
//    estágios serão chamados (IF, ID, EX, MEM, WB) sobre instruções
//    que já entraram no pipeline.
//  - "counterForEnd" controla quantos ciclos ainda faltam para esvaziar
//    o pipeline quando decidimos parar (drain).
//  - "clock" conta quantos ciclos o processo já rodou (usado com quantum).
//  - "endExecution" é setado quando o quantum acaba ou quando achamos
//    a instrução 'sentinela' para o fim de programa; então, começamos a decrementar
//    'counterForEnd' até 0 para garantir que todas instruções em execução terminem.
//  - "UC.data" é um buffer simples (FIFO implícito) onde empilhamos
//    "Instruction_Data" para acompanhar qual informação cada estágio deve usar.

void* Core(MainMemory &ram, PCB &process, vector<unique_ptr<ioRequest>>* ioRequests, bool &printLock) {
    auto &registers = process.regBank; // pega referência direta ao banco de registradores do processo
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
    // Enquanto houver instruções em execução no pipeline (counterForEnd > 0)
    while (context.counterForEnd > 0) {
        // WRITE BACK (estágio 5)
        // Só chamamos se já houver instrução suficiente no buffer
        // atenção ao índice: context.counter - 4
        if (context.counter >= 4 && context.counterForEnd >= 1) {
            UC.Write_Back(UC.data[context.counter - 4], context);
        }
        // MEMORY ACCESS (estágio 4)
        if (context.counter >= 3 && context.counterForEnd >= 2) {
            UC.Memory_Acess(UC.data[context.counter - 3], context);
        }
        // EXECUTE (estágio 3)
        if (context.counter >= 2 && context.counterForEnd >= 3) {
            UC.Execute(UC.data[context.counter - 2], context);
        }
        // DECODE (estágio 2)
        if (context.counter >= 1 && context.counterForEnd >= 4) {
            // Decode lê o IR (Instruction Register) atual do registro e preenche o Instruction_Data
            UC.Decode(context.registers, UC.data[context.counter - 1]);
        }
        // FETCH (estágio 1)
        // Só fazemos fetch de uma nova instrução enquanto estivermos aceitando
        // novas instruções (counterForEnd == 5 significa que pipeline está aberto)
        if (context.counter >= 0 && context.counterForEnd == 5) {
            // empurra um slot vazio no buffer e faz o fetch que grava IR (Instruction Register) no registrador
            UC.data.push_back(data);
            UC.Fetch(context);
            // Observe: Fetch escreve a IR; o Decode do próximo ciclo irá ler a IR
        }
        // avança o tempo do pipeline
        context.counter += 1;
        clock += 1;
        // se atingimos o quantum do processo, ou achamos END, sinalizar para terminar
        if (clock >= process.quantum || context.endProgram == true) {
            context.endExecution = true;
        }
        // quando endExecution estiver true começamos a decrementar counterForEnd
        // (Para terminar o pipeline). Isso garante que instruções já fetchadas terminem.
        if (context.endExecution == true) {
            context.counterForEnd -= 1;
        }
    }
    // se o fim do programa foi detectado, marcamos o processo como finalizado
    if (context.endProgram) {
        context.process.state = State::Finished;
    }

    // retorno nulo se quiser, pode retornar um status ou ponteiro mais informativo (não pensei nisso ainda).
    return nullptr;
}

