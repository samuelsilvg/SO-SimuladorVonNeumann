// control_unit_with_trace.cpp
#include "pcb_loader.hpp"
#include <fstream>
#include "CONTROL_UNIT.hpp"
#include "../memory/MemoryManager.hpp"
#include "PCB.hpp"
#include "../IO/IOManager.hpp"

#include <bitset>
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <vector>
#include <fstream>
#include <mutex>

static std::mutex log_mutex;



using namespace std;

void Control_Unit::log_operation(const std::string &msg) {
    std::lock_guard<std::mutex> lock(log_mutex);

    // Imprime no console
    std::cout << "[LOG] " << msg << std::endl;

    // Cria nome de arquivo temporário aleatório
    static int temp_file_id = 1;  // pode ser mais sofisticado
    std::ostringstream oss;
    oss << "output/temp_" << temp_file_id << ".log";

    std::ofstream fout(oss.str(), std::ios::app);
    if (fout.is_open()) {
        fout << msg << "\n";
    }
}


// Helpers
static uint32_t binaryStringToUint(const std::string &bin) {
    uint32_t value = 0;
    for (char c : bin) {
        value <<= 1;
        if (c == '1') value |= 1u;
        else if (c != '0') throw std::invalid_argument("binaryStringToUint: char nao binario");
    }
    return value;
}

static int32_t signExtend16(uint16_t v) {
    if (v & 0x8000)
        return (int32_t)(0xFFFF0000u | v);
    else
        return (int32_t)(v & 0x0000FFFFu);
}

static std::string regIndexToBitString(uint32_t idx) {
    std::string s(5, '0');
    for (int i = 4; i >= 0; --i) {
        s[4 - i] = ((idx >> i) & 1) ? '1' : '0';
    }
    return s;
}

static std::string toBinStr(uint32_t v, int width) {
    std::string s(width, '0');
    for (int i = 0; i < width; ++i)
        s[width - 1 - i] = ((v >> i) & 1) ? '1' : '0';
    return s;
}

static inline void account_pipeline_cycle(PCB &p) { p.pipeline_cycles.fetch_add(1); }
static inline void account_stage(PCB &p) { p.stage_invocations.fetch_add(1); }

string Control_Unit::Get_immediate(const uint32_t instruction) {
    uint16_t imm = static_cast<uint16_t>(instruction & 0xFFFFu);
    return std::bitset<16>(imm).to_string();
}

string Control_Unit::Get_destination_Register(const uint32_t instruction) {
    uint32_t rd = (instruction >> 11) & 0x1Fu;
    return regIndexToBitString(rd);
}

string Control_Unit::Get_target_Register(const uint32_t instruction) {
    uint32_t rt = (instruction >> 16) & 0x1Fu;
    return regIndexToBitString(rt);
}

string Control_Unit::Get_source_Register(const uint32_t instruction) {
    uint32_t rs = (instruction >> 21) & 0x1Fu;
    return regIndexToBitString(rs);
}

string Control_Unit::Identificacao_instrucao(uint32_t instruction, hw::REGISTER_BANK &registers) {
    (void)registers; // evita warning
    uint32_t opcode = (instruction >> 26) & 0x3Fu;
    std::string opcode_bin = toBinStr(opcode, 6);

    // Se existir mapa textual (instructionMap), mantenha compatibilidade
    for (const auto &p : instructionMap) {
        if (p.second == opcode_bin) {
            std::string key = p.first;
            for (auto &c : key) c = toupper(c);
            return key;
        }
    }

    // Tratamento por opcode numérico (MIPS-like / convenções comuns)
    switch (opcode) {
        case 0x00: { // R-type: usa funct
            uint32_t funct = instruction & 0x3Fu;
            if (funct == 0x20) return "ADD";
            if (funct == 0x22) return "SUB";
            if (funct == 0x18) return "MULT";
            if (funct == 0x1A) return "DIV";
            // não reconhecido -> vazio
            return "";
        }
        case 0x02: return "J";        // jump
        case 0x03: return "JAL";
        case 0x04: return "BEQ";
        case 0x05: return "BNE";
        case 0x08: return "ADDI";     // 001000
        case 0x09: return "ADDIU";    // 001001
        case 0x0F: return "LUI";      // 001111
        case 0x0C: return "ANDI";     // 001100 (opcional)
        case 0x0A: return "SLTI";     // 001010 (opcional)
        case 0x23: return "LW";       // 100011
        case 0x2B: return "SW";       // 101011
        case 0x0E: return "LI";       // custom LI, se usado
        case 0x10: return "PRINT";    // custom PRINT opcode, ajuste se necessário
        case 0x3F: return "END";      // sentinel/END (se aplicável)
        default:
            return ""; // desconhecido
    }
}

void Control_Unit::Fetch(ControlContext &context) {
    account_stage(context.process);
    // MAR <- PC
    context.registers.mar.write(context.registers.pc.value);
    // Read memory at MAR (endereçamento em bytes presunção: PC em bytes)
    uint32_t instr = context.memManager.read(context.registers.mar.read(), context.process);
    context.registers.ir.write(instr);

    // === TRACE FETCH ===
    std::cout << "[FETCH] PC=" << context.registers.pc.value
              << " MAR=" << context.registers.mar.read()
              << " INSTR=0x" << std::hex << instr << std::dec
              << " (" << toBinStr(instr, 32) << ")\n";

    const uint32_t END_SENTINEL = 0b11111100000000000000000000000000u;
    if (instr == END_SENTINEL) {
        context.endProgram = true;
        return;
    }
    // PC <- PC + 4 (endereçamento por byte)
    context.registers.pc.write(context.registers.pc.value + 4);
}

void Control_Unit::Decode(hw::REGISTER_BANK &registers, Instruction_Data &data) {
    uint32_t instruction = registers.ir.read();
    data.rawInstruction = instruction;
    data.op = Identificacao_instrucao(instruction, registers);

    // R-type
    if (data.op == "ADD" || data.op == "SUB" || data.op == "MULT" || data.op == "DIV") {
        data.source_register = Get_source_Register(instruction);
        data.target_register = Get_target_Register(instruction);
        data.destination_register = Get_destination_Register(instruction);

    // I-type: ADDI, ADDIU, LI, LW, LA, SW, and branches / custom immediates
    } else if (data.op == "ADDI" || data.op == "ADDIU" ||
               data.op == "LI" || data.op == "LW" || data.op == "LA" || data.op == "SW" ||
               data.op == "BGTI" || data.op == "BLTI" || data.op == "BEQ" || data.op == "BNE" ||
               data.op == "BGT"  || data.op == "BLT" || data.op == "SLTI" || data.op == "LUI") {

        data.source_register = Get_source_Register(instruction);   // rs
        data.target_register = Get_target_Register(instruction);   // rt (destino para ADDI/LW)
        data.addressRAMResult = Get_immediate(instruction);
        uint16_t imm16 = static_cast<uint16_t>(instruction & 0xFFFFu);
        data.immediate = signExtend16(imm16);

    } else if (data.op == "J") {
        uint32_t instr26 = instruction & 0x03FFFFFFu;
        data.addressRAMResult = std::bitset<26>(instr26).to_string();
        data.immediate = static_cast<int32_t>(instr26);
    } else if (data.op == "PRINT") {
        data.target_register = Get_target_Register(instruction);
        std::string imm = Get_immediate(instruction);
        bool allZero = true;
        for (char c : imm) if (c != '0') { allZero = false; break; }
        if (!allZero) {
            data.addressRAMResult = imm;
            uint16_t imm16 = static_cast<uint16_t>(binaryStringToUint(imm));
            data.immediate = signExtend16(imm16);
        } else {
            data.addressRAMResult.clear();
            data.immediate = 0;
        }
    }

    // === TRACE DECODE ===
    std::cout << "[DECODE] RAW=0x" << std::hex << data.rawInstruction << std::dec
              << " OP=" << (data.op.empty() ? "<UNKNOWN>" : data.op) << "\n";
    if (!data.source_register.empty()) {
        std::cout << "         rs(bits)=" << data.source_register
                  << " name=" << this->map.getRegisterName(binaryStringToUint(data.source_register)) << "\n";
    }
    if (!data.target_register.empty()) {
        std::cout << "         rt(bits)=" << data.target_register
                  << " name=" << this->map.getRegisterName(binaryStringToUint(data.target_register)) << "\n";
    }
    if (!data.destination_register.empty()) {
        std::cout << "         rd(bits)=" << data.destination_register
                  << " name=" << this->map.getRegisterName(binaryStringToUint(data.destination_register)) << "\n";
    }
    if (!data.addressRAMResult.empty()) {
        std::cout << "         address/immediate(bits)=" << data.addressRAMResult
                  << " immediate(signed)=" << data.immediate << "\n";
    }
}


void Control_Unit::Execute_Immediate_Operation(hw::REGISTER_BANK &registers, Instruction_Data &data) {
    std::string name_rs = this->map.getRegisterName(binaryStringToUint(data.source_register));
    std::string name_rt = this->map.getRegisterName(binaryStringToUint(data.target_register));

    int32_t val_rs = registers.readRegister(name_rs);
    int32_t imm = data.immediate; // já sign-extended

    std::ostringstream ss;

    // ADDI / ADDIU
    if (data.op == "ADDI" || data.op == "ADDIU") {
        ALU alu;
        alu.A = val_rs;
        alu.B = imm;
        alu.op = ADD;
        alu.calculate();
        registers.writeRegister(name_rt, alu.result);

        ss << "[IMM] " << data.op << " "
           << name_rt << " = " << name_rs << "(" << val_rs << ") + "
           << imm << " -> " << alu.result;
        log_operation(ss.str());
        return;
    }

    // SLTI
    if (data.op == "SLTI") {
        int32_t res = (val_rs < imm) ? 1 : 0;
        registers.writeRegister(name_rt, res);

        ss << "[IMM] SLTI " << name_rt << " = (" << name_rs << "(" << val_rs
           << ") < " << imm << ") ? 1 : 0 -> " << res;
        log_operation(ss.str());
        return;
    }

    // LUI
    if (data.op == "LUI") {
        uint32_t uimm = static_cast<uint32_t>(static_cast<uint16_t>(imm));
        int32_t val = static_cast<int32_t>(uimm << 16);
        registers.writeRegister(name_rt, val);

        ss << "[IMM] LUI " << name_rt << " = (0x" << std::hex << imm
           << " << 16) -> 0x" << val << std::dec;
        log_operation(ss.str());
        return;
    }

    // LI
    if (data.op == "LI") {
        registers.writeRegister(name_rt, imm);

        ss << "[IMM] LI " << name_rt << " = " << imm;
        log_operation(ss.str());
        return;
    }

    // Caso não mapeado
    ss << "[IMM] UNKNOWN OP: " << data.op
       << " rs=" << name_rs << " imm=" << imm;
    log_operation(ss.str());
}

void Control_Unit::Execute_Aritmetic_Operation(hw::REGISTER_BANK &registers, Instruction_Data &data) {
    std::string name_rs = this->map.getRegisterName(binaryStringToUint(data.source_register));
    std::string name_rt = this->map.getRegisterName(binaryStringToUint(data.target_register));
    std::string name_rd = this->map.getRegisterName(binaryStringToUint(data.target_register));

    int32_t val_rs = registers.readRegister(name_rs);
    int32_t val_rt = registers.readRegister(name_rt);

    ALU alu;
    alu.A = val_rs;
    alu.B = val_rt;

    if (data.op == "ADD") alu.op = ADD;
    else if (data.op == "SUB") alu.op = SUB;
    else if (data.op == "MULT") alu.op = MUL;
    else if (data.op == "DIV") alu.op = DIV;
    else return;

    alu.calculate();
    registers.writeRegister(name_rd, alu.result);

    std::ostringstream ss;
    ss << "[ARIT] " << data.op << " " << name_rd
       << " = " << name_rs << "(" << val_rs << ") "
       << data.op << " " << name_rt << "(" << val_rt << ") = "
       << alu.result;
    log_operation(ss.str());
}

void Control_Unit::Execute_Operation(Instruction_Data &data, ControlContext &context) {
    if (data.op == "PRINT") {
        if (!data.target_register.empty()) {
            string name = this->map.getRegisterName(binaryStringToUint(data.target_register));
            int value = context.registers.readRegister(name);
            auto req = std::make_unique<IORequest>();
            req->msg = std::to_string(value);
            req->process = &context.process;
            context.ioRequests.push_back(std::move(req));

            // TRACE PRINT from register
            std::cout << "[PRINT-REQ] PRINT REG " << name << " value=" << value
                      << " (pid=" << context.process.pid << ")\n";

            if (context.printLock) {
                context.process.state = State::Blocked;
                context.endExecution = true;
            }
        }
    }
}

void Control_Unit::Execute_Loop_Operation(hw::REGISTER_BANK &registers, Instruction_Data &data,
                                          int &counter, int &counterForEnd, bool &programEnd,
                                          MemoryManager &memManager, PCB &process) {
    string name_rs = this->map.getRegisterName(binaryStringToUint(data.source_register));
    string name_rt = this->map.getRegisterName(binaryStringToUint(data.target_register));

    ALU alu;
    alu.A = registers.readRegister(name_rs);
    alu.B = registers.readRegister(name_rt);

    bool jump = false;
    if (data.op == "BEQ") { alu.op = BEQ; alu.calculate(); if (alu.result == 1) jump = true; }
    else if (data.op == "BNE") { alu.op = BNE; alu.calculate(); if (alu.result == 1) jump = true; }
    else if (data.op == "J") { jump = true; }
    else if (data.op == "BLT") { alu.op = BLT; alu.calculate(); if (alu.result == 1) jump = true; }
    else if (data.op == "BGT") { alu.op = BGT; alu.calculate(); if (alu.result == 1) jump = true; }

    if (jump) {
        uint32_t addr = binaryStringToUint(data.addressRAMResult);
        // TRACE BRANCH/JUMP
        std::cout << "[BRANCH] OP=" << data.op << " taken, new PC=" << addr << "\n";

        registers.pc.write(addr);
        registers.ir.write(memManager.read(registers.pc.read(), process));
        counter = 0; counterForEnd = 5; programEnd = false;
    }
}

void Control_Unit::Execute(Instruction_Data &data, ControlContext &context) {
    account_stage(context.process);

    // Immediates / I-type arithmetic
    if (data.op == "ADDI" || data.op == "ADDIU" || data.op == "SLTI" || data.op == "LUI" || data.op == "LI") {
        Execute_Immediate_Operation(context.registers, data);
        return;
    }

    // R-type
    if (data.op == "ADD" || data.op == "SUB" || data.op == "MULT" || data.op == "DIV") {
        Execute_Aritmetic_Operation(context.registers, data);
    } else if (data.op == "BEQ" || data.op == "J" || data.op == "BNE" || data.op == "BGT" || data.op == "BGTI" || data.op == "BLT" || data.op == "BLTI") {
        Execute_Loop_Operation(context.registers, data, context.counter, context.counterForEnd, context.endProgram, context.memManager, context.process);
    } else if (data.op == "PRINT") {
        Execute_Operation(data, context);
    }
}

void Control_Unit::Memory_Acess(Instruction_Data &data, ControlContext &context) {
    account_stage(context.process);
    string name_rt = this->map.getRegisterName(binaryStringToUint(data.target_register));
    if (data.op == "LW") {
        uint32_t addr = binaryStringToUint(data.addressRAMResult);
        int value = context.memManager.read(addr, context.process);
        context.registers.writeRegister(name_rt, value);

        std::cout << "[MEMORY] LW addr=" << addr << " value=" << value
                  << " -> " << name_rt << "\n";
    } else if (data.op == "LA" || data.op == "LI") {
        uint32_t val = binaryStringToUint(data.addressRAMResult);
        context.registers.writeRegister(name_rt, static_cast<int>(val));

        std::cout << "[MEMORY] " << data.op << " -> " << name_rt
                  << " value=" << static_cast<int>(val) << "\n";
    } else if (data.op == "PRINT" && data.target_register.empty()) {
        uint32_t addr = binaryStringToUint(data.addressRAMResult);
        int value = context.memManager.read(addr, context.process);
        auto req = std::make_unique<IORequest>();
        req->msg = std::to_string(value);
        req->process = &context.process;
        context.ioRequests.push_back(std::move(req));

        std::cout << "[PRINT-REQ] PRINT MEM addr=" << addr << " value=" << value
                  << " (pid=" << context.process.pid << ")\n";

        if (context.printLock) {
            context.process.state = State::Blocked;
            context.endExecution = true;
        }
    }
}

void Control_Unit::Write_Back(Instruction_Data &data, ControlContext &context) {
    account_stage(context.process);
    if (data.op == "SW") {
        uint32_t addr = binaryStringToUint(data.addressRAMResult);
        string name_rt = this->map.getRegisterName(binaryStringToUint(data.target_register));
        int value = context.registers.readRegister(name_rt);
        context.memManager.write(addr, value, context.process);

        std::cout << "[WRITE-BACK] SW addr=" << addr << " value=" << value
                  << " from reg " << name_rt << "\n";
    }
}

// A função Core agora espera um ponteiro para o PCB, pois o PCB não é mais copiável
void* Core(MemoryManager &memoryManager, PCB &process, vector<unique_ptr<IORequest>>* ioRequests, bool &printLock) {
    Control_Unit UC;
    Instruction_Data data;
    int clock = 0;
    int counterForEnd = 5;
    int counter = 0;
    bool endProgram = false;
    bool endExecution = false;

    ControlContext context{ process.regBank, memoryManager, *ioRequests, printLock, process, counter, counterForEnd, endProgram, endExecution };

    while (context.counterForEnd > 0) {
        if (context.counter >= 4 && context.counterForEnd >= 1) {
            UC.Write_Back(UC.data[context.counter - 4], context);
        }
        if (context.counter >= 3 && context.counterForEnd >= 2) {
            UC.Memory_Acess(UC.data[context.counter - 3], context);
        }
        if (context.counter >= 2 && context.counterForEnd >= 3) {
            UC.Execute(UC.data[context.counter - 2], context);
        }
        if (context.counter >= 1 && context.counterForEnd >= 4) {
            account_stage(process);
            UC.Decode(context.registers, UC.data[context.counter - 1]);
        }
        if (context.counter >= 0 && context.counterForEnd == 5) {
            UC.data.push_back(data);
            UC.Fetch(context);
        }

        context.counter += 1;
        clock += 1;
        account_pipeline_cycle(process);

        if (clock >= process.quantum || context.endProgram == true) {
            context.endExecution = true;
        }
        if (context.endExecution == true) {
            context.counterForEnd -= 1;
        }
    }

    if (context.endProgram) {
        process.state = State::Finished;
    }

    // === DUMP FINAL DOS REGISTRADORES ===
    {
        // nomes comuns de registradores MIPS como fallback
        const vector<string> fallback_names = {
            "zero","at","v0","v1","a0","a1","a2","a3",
            "t0","t1","t2","t3","t4","t5","t6","t7",
            "s0","s1","s2","s3","s4","s5","s6","s7",
            "t8","t9","k0","k1","gp","sp","fp","ra"
        };

        std::cout << "\n=== DUMP FINAL DE REGISTRADORES (PID=" << process.pid << ") ===\n";

        // Primeiro tentamos usar um mapper global se existir; caso contrário, usamos os nomes de fallback
        bool printed = false;
        try {
            // Se sua infra tem um mapper global com getRegisterName(uint32_t), descomente a linha abaixo:
            // auto &mapper = hw::getGlobalRegisterMapper();
            // for (uint32_t i = 0; i < 32; ++i) {
            //     std::string name = mapper.getRegisterName(i);
            //     int val = context.registers.readRegister(name);
            //     std::cout << name << " (" << i << ") = " << val << "\n";
            // }
            // printed = true;

            // Se não tiver mapper global, usamos nomes de fallback:
            for (uint32_t i = 0; i < 32; ++i) {
                string name;
                if (i < fallback_names.size()) name = fallback_names[i];
                else name = "r" + to_string(i);
                int val = context.registers.readRegister(name);
                std::cout << name << " (" << i << ") = " << val << "\n";
            }
            printed = true;
        } catch (...) {
            // caso algo dê errado ao ler por nome, apenas tentamos imprimir PC/IR
            printed = false;
        }

        // PC e IR
        std::cout << "PC = " << context.registers.pc.read() << "\n";
        std::cout << "IR = 0x" << std::hex << context.registers.ir.read() << std::dec
                  << " (" << toBinStr(context.registers.ir.read(), 32) << ")\n";
        std::cout << "========================================\n\n";
    }

    return nullptr;
}
