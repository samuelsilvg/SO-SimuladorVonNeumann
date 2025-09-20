#include "ULA.hpp"
#include <limits>

/*
  ULA.hpp (Provisório - JP)
  Observações:
  - Interface intencionalmente minimalista: 
    campos públicos A, B, result, overflow, op e método calculate().
  - Todos os cálculos são realizados interpretando A e B como inteiros com sinal
    de 32 bits (int32_t) quando aplicável (operações aritméticas e comparações).
  - Para detectar overflow aritmético usei aritmética de 64 bits (int64_t)
    como intermediário e chequei se o resultado cabe em 32 bits.
  - Para operações de comparação (BEQ/BNE/BLT/BGT e variantes com imediato),
    o campo `result` assume 1 quando a condição é verdadeira e 0 caso contrário.
  - Para operações de memória (LW, LA, ST) a ULA retorna o endereço efetivo
    calculado normalmente como base + offset.
*/

void ALU::calculate(){

    // reset flags/resultado antes de cada execução
    overflow = false;
    result = 0;

    // helpers inline (sem poluição do namespace global)
    auto as_i32 = [](uint32_t x) -> int32_t
    { return static_cast<int32_t>(x); };
    auto fits_in_i32 = [](int64_t x) -> bool
    {
        return (x <= std::numeric_limits<int32_t>::max()) &&
               (x >= std::numeric_limits<int32_t>::min());
    };

    switch (op)
    {
    case ADD:
    {
        // soma com detecção de overflow (signed)
        int64_t res64 = static_cast<int64_t>(as_i32(A)) + static_cast<int64_t>(as_i32(B));
        if (!fits_in_i32(res64))
        {
            overflow = true;
        }
        result = static_cast<int32_t>(res64);
        break;
    }

    case SUB:
    {
        // subtração com detecção de overflow (signed)
        int64_t res64 = static_cast<int64_t>(as_i32(A)) - static_cast<int64_t>(as_i32(B));
        if (!fits_in_i32(res64))
        {
            overflow = true;
        }
        result = static_cast<int32_t>(res64);
        break;
    }

    case MUL:
    {
        // multiplicação com detecção de overflow (signed)
        int64_t res64 = static_cast<int64_t>(as_i32(A)) * static_cast<int64_t>(as_i32(B));
        if (!fits_in_i32(res64))
        {
            overflow = true;
        }
        result = static_cast<int32_t>(res64);
        break;
    }

    case DIV:
    {
        // divisão (signed). Tratar divisão por zero.
        int32_t divisor = as_i32(B);
        if (divisor == 0)
        {
            // Convenção: sinaliza overflow/erro de divisão por zero e
            // definimos resultado como 0. Alternativa: lançar exceção.
            overflow = true;
            result = 0;
        }
        else
        {
            // cuidado com INT_MIN / -1 -> overflow (porque |INT_MIN| > INT_MAX)
            int32_t dividend = as_i32(A);
            if (dividend == std::numeric_limits<int32_t>::min() && divisor == -1)
            {
                overflow = true;
                // resultado matematicamente é 2147483648, mas não cabe em int32
                result = std::numeric_limits<int32_t>::min(); // convenção
            }
            else
            {
                result = dividend / divisor;
            }
        }
        break;
    }

    case AND_OP:
    {
        // Operação bit a bit AND (interpretamos A e B como unsigned aqui)
        result = static_cast<int32_t>(A & B);
        break;
    }

    case BEQ:
    {
        // Branch if equal: result = 1 se A == B, senão 0
        result = (as_i32(A) == as_i32(B)) ? 1 : 0;
        break;
    }

    case BNE:
    {
        // Branch if not equal
        result = (as_i32(A) != as_i32(B)) ? 1 : 0;
        break;
    }

    case BLT:
    {
        // Branch if less than (signed)
        result = (as_i32(A) < as_i32(B)) ? 1 : 0;
        break;
    }

    case BGT:
    {
        // Branch if greater than (signed)
        result = (as_i32(A) > as_i32(B)) ? 1 : 0;
        break;
    }

    case BGTI:
    {
        // Branch if greater than immediate
        // Convenção: B contém o imediato; compara A > B
        result = (as_i32(A) > as_i32(B)) ? 1 : 0;
        break;
    }

    case BLTI:
    {
        // Branch if less than immediate
        // Convenção: B contém o imediato; compara A < B
        result = (as_i32(A) < as_i32(B)) ? 1 : 0;
        break;
    }

    case LW:
    case LA:
    case ST:
    {
        // Operações de memória: calculam endereço efetivo = base + offset
        // (A = base, B = offset/imediato). Usei aritmética signed para
        // permitir offsets negativos.
        int64_t addr64 = static_cast<int64_t>(as_i32(A)) + static_cast<int64_t>(as_i32(B));
        if (!fits_in_i32(addr64))
        {
            overflow = true;
        }
        result = static_cast<int32_t>(addr64);
        break;
    }

    default:
    {
        // Caso não esperado: definir resultado 0 e sinalizar overflow = true
        overflow = true;
        result = 0;
        break;
    }
    } // switch
} // calculate()

void ALU::execute(operation ALUop, uint32_t a, uint32_t b, uint32_t shamt){
    //Carregando as entradas.
    op = ALUop;
    A = a;
    B = b;
    calculate();
}

