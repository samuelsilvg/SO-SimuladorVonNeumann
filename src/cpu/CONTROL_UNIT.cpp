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

// CORRIGIDO: hw::BANK alterado para hw::REGISTER_BANK
string Control_Unit::Identificacao_instrucao(uint32_t instruction, hw::REGISTER_BANK &registers) {
    (void)registers; // Parâmetro não utilizado, evita warning
    uint32_t opcode = (instruction >> 26) & 0x3Fu;
    std::string opcode_bin = toBinStr(opcode, 6);

    for (const auto &p : instructionMap) {
        if (p.second == opcode_bin) {
            std::string key = p.first;
            for (auto &c : key) c = toupper(c);
            return key;
        }
    }
    if (opcode == 0) {
        uint32_t funct = instruction & 0x3Fu;
        if (funct == 0x20) return "ADD";
        if (funct == 0x22) return "SUB";
        if (funct == 0x18) return "MULT";
        if (funct == 0x1A) return "DIV";
    }
    return "";
}

void Control_Unit::Fetch(ControlContext &context) {
    account_stage(context.process);
    context.registers.mar.write(context.registers.pc.value);
    uint32_t instr = context.memManager.read(context.registers.mar.read(), context.process);
    context.registers.ir.write(instr);

    const uint32_t END_SENTINEL = 0b11111100000000000000000000000000u;
    if (instr == END_SENTINEL) {
        context.endProgram = true;
        return;
    }
    context.registers.pc.write(context.registers.pc.value + 1);
}

void Control_Unit::Decode(hw::REGISTER_BANK &registers, Instruction_Data &data) {
    uint32_t instruction = registers.ir.read();
    data.rawInstruction = instruction;
    data.op = Identificacao_instrucao(instruction, registers);

    if (data.op == "ADD" || data.op == "SUB" || data.op == "MULT" || data.op == "DIV") {
        data.source_register = Get_source_Register(instruction);
        data.target_register = Get_target_Register(instruction);
        data.destination_register = Get_destination_Register(instruction);
    } else if (data.op == "LI" || data.op == "LW" || data.op == "LA" || data.op == "SW") {
        data.target_register = Get_target_Register(instruction);
        data.addressRAMResult = Get_immediate(instruction);
        uint16_t imm16 = static_cast<uint16_t>(instruction & 0xFFFFu);
        data.immediate = signExtend16(imm16);
    } else if (data.op == "BLT" || data.op == "BGT" || data.op == "BGTI" || data.op == "BLTI" || data.op == "BEQ" || data.op == "BNE") {
        data.source_register = Get_source_Register(instruction);
        data.target_register = Get_target_Register(instruction);
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
}

// CORRIGIDO: hw::BANK alterado para hw::REGISTER_BANK
void Control_Unit::Execute_Aritmetic_Operation(hw::REGISTER_BANK &registers, Instruction_Data &data) {
    string name_rs = this->map.getRegisterName(binaryStringToUint(data.source_register));
    string name_rt = this->map.getRegisterName(binaryStringToUint(data.target_register));
    string name_rd = this->map.getRegisterName(binaryStringToUint(data.destination_register));

    ALU alu;
    alu.A = registers.readRegister(name_rs);
    alu.B = registers.readRegister(name_rt);

    if (data.op == "ADD") alu.op = ADD;
    else if (data.op == "SUB") alu.op = SUB;
    else if (data.op == "MULT") alu.op = MUL;
    else if (data.op == "DIV") alu.op = DIV;

    alu.calculate();
    registers.writeRegister(name_rd, alu.result);
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

            if (context.printLock) {
                context.process.state = State::Blocked;
                context.endExecution = true;
            }
        }
    }
}

// CORRIGIDO: hw::BANK alterado para hw::REGISTER_BANK
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
        registers.pc.write(addr);
        registers.ir.write(memManager.read(registers.pc.read(), process));
        counter = 0; counterForEnd = 5; programEnd = false;
    }
}

void Control_Unit::Execute(Instruction_Data &data, ControlContext &context) {
    account_stage(context.process);
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
    } else if (data.op == "LA" || data.op == "LI") {
        uint32_t val = binaryStringToUint(data.addressRAMResult);
        context.registers.writeRegister(name_rt, static_cast<int>(val));
    } else if (data.op == "PRINT" && data.target_register.empty()) {
        uint32_t addr = binaryStringToUint(data.addressRAMResult);
        int value = context.memManager.read(addr, context.process);
        auto req = std::make_unique<IORequest>();
        req->msg = std::to_string(value);
        req->process = &context.process;
        context.ioRequests.push_back(std::move(req));
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

    return nullptr;
}