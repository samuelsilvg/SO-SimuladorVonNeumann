#ifndef ULA_HPP
#define ULA_HPP
#include <cstdint>

enum operation
{
    ADD,
    SUB,
    MUL,
    DIV,
    AND_OP,
    BEQ,
    BNE,
    BLT,
    BGT,
    BGTI, // BGTI immediate - Compara A > B (B é usado como imediato)
    BLTI, // BLTI immediate - Compara A < B (B é usado como imediato)
    LW,   // Load word - calcula endereço (base + offset)
    LA,   // Load adress - similar a LW (retorna enredeço efetivo)
    ST    // Store - calcula endereço para gravação (base + offset)
};

class ALU
{
public:
    // -> Entradas
    uint32_t A = 0;
    uint32_t B = 0;
    operation op = ADD;

    // -> Saídas
    int32_t result = 0;    // Resultado (interpretração: signed 32-bit na maioria dos casos)
    bool overflow = false; // Indica overflow aritmético (ou erro, como divisão por zero)

    // -> Métodos principais
    void calculate();
    void execute(operation ALUop, uint32_t a, uint32_t b, uint32_t shamt = 0);
};
#endif // ULA_HPP