#ifndef CONTROL_UNIT_HPP
#define CONTROL_UNIT_HPP

#include "REGISTER_BANK.hpp" // Incluído diretamente para ter a definição completa
#include "ULA.hpp"
#include "HASH_REGISTER.hpp"
#include "../memory/cache.hpp"
#include <unordered_map>
#include <string>
#include <vector>
#include <cstdint>
#include <memory>

using std::string;
using std::vector;
using std::uint32_t;
using std::unique_ptr;

// Forward declarations
class MemoryManager;
struct PCB;
struct IORequest;

void* Core(MemoryManager &memoryManager, PCB &process, vector<unique_ptr<IORequest>>* ioRequests, bool &printLock);

struct Instruction_Data {
    string source_register;
    string target_register;
    string destination_register;
    string op;
    string addressRAMResult;
    uint32_t rawInstruction = 0;
    int32_t immediate = 0;
};

struct ControlContext {
    hw::REGISTER_BANK &registers;
    MemoryManager &memManager;
    vector<unique_ptr<IORequest>> &ioRequests;
    bool &printLock;
    PCB &process;
    int &counter;
    int &counterForEnd;
    bool &endProgram;
    bool &endExecution;
};

struct Control_Unit {
    vector<Instruction_Data> data;
    hw::Map map;

    std::unordered_map<string, string> instructionMap = {
        {"add", "000000"}, {"and", "000001"}, {"div", "000010"}, {"mult","000011"},
        {"sub", "000100"}, {"beq", "000101"}, {"bne", "000110"}, {"bgt", "000111"},
        {"bgti","001000"}, {"blt", "001001"}, {"blti","001010"}, {"j", "001011"},
        {"lw", "001100"},  {"sw", "001101"},  {"li", "001110"},  {"la", "001111"},
        {"print", "010000"},{"end", "111111"}
    };

    static string Get_immediate(uint32_t instruction);
    static string Get_destination_Register(uint32_t instruction);
    static string Get_target_Register(uint32_t instruction);
    static string Get_source_Register(uint32_t instruction);

    // Assinatura corrigida para corresponder à implementação
    string Identificacao_instrucao(uint32_t instruction, hw::REGISTER_BANK &registers);

    void Fetch(ControlContext &context);
    void Decode(hw::REGISTER_BANK &registers, Instruction_Data &data);
    void Execute_Aritmetic_Operation(hw::REGISTER_BANK &registers, Instruction_Data &d);
    void Execute_Operation(Instruction_Data &data, ControlContext &context);
    void Execute_Loop_Operation(hw::REGISTER_BANK &registers, Instruction_Data &d,
                                int &counter, int &counterForEnd, bool &endProgram,
                                MemoryManager &memManager, PCB &process);
    void Execute(Instruction_Data &data, ControlContext &context);
    void Memory_Acess(Instruction_Data &data, ControlContext &context);
    void Write_Back(Instruction_Data &data, ControlContext &context);
};

#endif // CONTROL_UNIT_HPP