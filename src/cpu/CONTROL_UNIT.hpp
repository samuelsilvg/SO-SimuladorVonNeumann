#ifndef CONTROL_UNIT_HPP
#define CONTROL_UNIT_HPP

/*
  CONTROL_UNIT.hpp (provisório - JP)
  Métodos que acessam memória,
  I/O ou o banco de registradores permanecem apenas com assinaturas — a
  implementação fica no .cpp, onde você conhece as APIs concretas.
*/

#include "ULA.hpp"
#include "REGISTER_BANK.hpp"
#include "HASH_REGISTER.hpp"     // contém 'Map' (mapa código->nome do registrador)
#include <unordered_map>
#include <string>
#include <vector>
#include <cstdint>
#include <memory>

using std::string;
using std::vector;
using std::uint32_t;
using std::unique_ptr;

/* Forward declarations (mantém o header leve; inclua os headers reais no .cpp) */
class MainMemory;
struct PCB;
struct ioRequest;

/* Função principal do core (assinatura dependente da memória) */
void* Core(MainMemory &ram, PCB &process, vector<unique_ptr<ioRequest>>* ioRequests, bool &printLock);

/* Estrutura para guardar dados da instrução entre estágios */
struct Instruction_Data {
    string source_register;      // ex: "t0"
    string target_register;      // ex: "t1"
    string destination_register; // ex: "t2"
    string op;                   // mnemonic: "add", "lw", "beq", ...
    string addressRAMResult;     // endereço calculado (opcional)
    uint32_t rawInstruction = 0; // palavra bruta (32 bits)
    int32_t immediate = 0;       // imediato sign-extended
};

/* Contexto de execução passado para métodos que precisam de recursos */
struct ControlContext {
    REGISTER_BANK &registers;
    MainMemory &ram;
    vector<unique_ptr<ioRequest>> &ioRequests;
    bool &printLock;
    PCB &process;
    int &counter;
    int &counterForEnd;
    bool &endProgram;
    bool &endExecution;
};

/* Unidade de Controle */
struct Control_Unit {
    operation op;                    // operação atual (do enum em ULA.hpp)
    vector<Instruction_Data> data;   // buffer simples de insstruções
    Map map;                         // mapa de códigos -> nomes de registradores

    // mnemônicos -> opcode (string binária ou código simbólico)
    std::unordered_map<string, string> instructionMap = {
        {"add", "000000"}, {"and", "000001"}, {"div", "000010"}, {"mult","000011"},
        {"sub", "000100"}, {"beq", "000101"}, {"bne", "000110"}, {"bgt", "000111"},
        {"bgti","001000"}, {"blt", "001001"}, {"blti","001010"}, {"j", "001011"},
        {"lw", "001100"},  {"sw", "001101"},  {"li", "001110"},  {"la", "001111"},
        {"print", "010000"},{"end", "111111"}
    };

    // --- utilitários de extração (apenas assinaturas; implementar no .cpp) ---
    // Extraem campos de uma instrução de 32 bits (MIPS-like).
    string Get_immediate(const uint32_t instruction);
    string Pick_Code_Register_Load(const uint32_t instruction);
    string Get_destination_Register(const uint32_t instruction);
    string Get_target_Register(const uint32_t instruction);
    string Get_source_Register(const uint32_t instruction);

    // Identifica o mnemonic a partir da palavra de 32 bits (opcode/funct)
    string Identificacao_instrucao(uint32_t instruction, REGISTER_BANK &registers);

    // --- ciclo básico: Fetch, Decode, Execute, Memory, WriteBack ---
    void Fetch(ControlContext &context);                                            // busca instrução da memória
    void Decode(REGISTER_BANK &registers, Instruction_Data &data);                  // decodifica campos
    void Execute_Aritmetic_Operation(REGISTER_BANK &registers, Instruction_Data &d); // usa ULA para ALU-ops
    void Execute_Operation(Instruction_Data &data, ControlContext &context);       // branches / jumps / syscalls
    void Execute_Loop_Operation(REGISTER_BANK &registers, Instruction_Data &d, int &counter, int &counterForEnd, bool& endProgram, MainMemory& ram);
    void Execute(Instruction_Data &data, ControlContext &context);                 // dispatcher de execução
    void Memory_Acess(Instruction_Data &data, ControlContext &context);           // LW / SW (depende de MainMemory)
    void Write_Back(Instruction_Data &data, ControlContext &context);             // grava resultado no banco de registradores
};

#endif // CONTROL_UNIT_HPP
