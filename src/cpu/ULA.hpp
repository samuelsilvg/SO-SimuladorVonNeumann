#ifndef ULA_HPP
#define ULA_HPP

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

#include <cstdint>
#include <limits>

enum operation {
    ADD,
    SUB,
    MUL,
    DIV,
    BEQ,
    BNE,
    BLT,
    BGT,
    BGTI,   // BGT immediate — compara A > B (B é usado como imediato)
    BLTI,   // BLT immediate — compara A < B (B é usado como imediato)
    LW,     // Load word - calcula endereço (base + offset)
    LA,     // Load address - similar a LW (retorna endereço efetivo)
    ST      // Store - calcula endereço para gravação (base + offset)
};

struct ALU {
    // Entradas
    uint32_t A = 0;      // Operando A (normalmente registrador base)
    uint32_t B = 0;      // Operando B (registrador ou imediato)
    operation op = ADD;  // Operação a ser executada

    // Saídas
    int32_t result = 0;  // Resultado (interpretação: signed 32-bit na maioria dos casos)
    bool overflow = false; // Indica overflow aritmético / erro (por ex. divisão por zero)

    /*
      calculate()
      - Executa a operação definida em `op` usando A e B como operandos.
      - Atualiza `result` e `overflow`.
      - Convenções:
        * A e B são interpretados como signed int32_t para operações aritméticas
          e de comparação.
        * Para comparações (BEQ, BNE, BLT, BGT, BGTI, BLTI) `result` recebe 1
          quando a condição é verdadeira, 0 caso contrário. `overflow` = false.
        * Para DIV, se B == 0 -> `overflow` = true e `result` = 0 (não lança).
        * Para LW/LA/ST: calcula endereço efetivo = (int32_t)A + (int32_t)B.
          `overflow` será true se esse somatório ultrapassar limites de int32.
    */
    void calculate() {
        // reset flags/resultado antes de cada execução
        overflow = false;
        result = 0;

        // helpers inline (sem poluição do namespace global)
        auto as_i32 = [](uint32_t x) -> int32_t { return static_cast<int32_t>(x); };
        auto fits_in_i32 = [](int64_t x) -> bool {
            return (x <= std::numeric_limits<int32_t>::max()) &&
                   (x >= std::numeric_limits<int32_t>::min());
        };

        switch (op) {
            case ADD: {
                // soma com detecção de overflow (signed)
                int64_t res64 = static_cast<int64_t>(as_i32(A)) + static_cast<int64_t>(as_i32(B));
                if (!fits_in_i32(res64)) {
                    overflow = true;
                }
                result = static_cast<int32_t>(res64);
                break;
            }

            case SUB: {
                // subtração com detecção de overflow (signed)
                int64_t res64 = static_cast<int64_t>(as_i32(A)) - static_cast<int64_t>(as_i32(B));
                if (!fits_in_i32(res64)) {
                    overflow = true;
                }
                result = static_cast<int32_t>(res64);
                break;
            }

            case MUL: {
                // multiplicação com detecção de overflow (signed)
                int64_t res64 = static_cast<int64_t>(as_i32(A)) * static_cast<int64_t>(as_i32(B));
                if (!fits_in_i32(res64)) {
                    overflow = true;
                }
                result = static_cast<int32_t>(res64);
                break;
            }

            case DIV: {
                // divisão (signed). Tratar divisão por zero.
                int32_t divisor = as_i32(B);
                if (divisor == 0) {
                    // Convenção: sinaliza overflow/erro de divisão por zero e
                    // definimos resultado como 0. Alternativa: lançar exceção.
                    overflow = true;
                    result = 0;
                } else {
                    // cuidado com INT_MIN / -1 -> overflow (porque |INT_MIN| > INT_MAX)
                    int32_t dividend = as_i32(A);
                    if (dividend == std::numeric_limits<int32_t>::min() && divisor == -1) {
                        overflow = true;
                        // resultado matematicamente é 2147483648, mas não cabe em int32
                        result = std::numeric_limits<int32_t>::min(); // convenção
                    } else {
                        result = dividend / divisor;
                    }
                }
                break;
            }

            case BEQ: {
                // Branch if equal: result = 1 se A == B, senão 0
                result = (as_i32(A) == as_i32(B)) ? 1 : 0;
                break;
            }

            case BNE: {
                // Branch if not equal
                result = (as_i32(A) != as_i32(B)) ? 1 : 0;
                break;
            }

            case BLT: {
                // Branch if less than (signed)
                result = (as_i32(A) < as_i32(B)) ? 1 : 0;
                break;
            }

            case BGT: {
                // Branch if greater than (signed)
                result = (as_i32(A) > as_i32(B)) ? 1 : 0;
                break;
            }

            case BGTI: {
                // Branch if greater than immediate
                // Convenção: B contém o imediato; compara A > B
                result = (as_i32(A) > as_i32(B)) ? 1 : 0;
                break;
            }

            case BLTI: {
                // Branch if less than immediate
                // Convenção: B contém o imediato; compara A < B
                result = (as_i32(A) < as_i32(B)) ? 1 : 0;
                break;
            }

            case LW:
            case LA:
            case ST: {
                // Operações de memória: calculam endereço efetivo = base + offset
                // (A = base, B = offset/imediato). Usei aritmética signed para
                // permitir offsets negativos.
                int64_t addr64 = static_cast<int64_t>(as_i32(A)) + static_cast<int64_t>(as_i32(B));
                if (!fits_in_i32(addr64)) {
                    overflow = true;
                }
                result = static_cast<int32_t>(addr64);
                break;
            }

            default: {
                // Caso não esperado: definir resultado 0 e sinalizar overflow = true
                overflow = true;
                result = 0;
                break;
            }
        } // switch
    } // calculate()
}; // struct ALU

#endif // ULA_HPP
