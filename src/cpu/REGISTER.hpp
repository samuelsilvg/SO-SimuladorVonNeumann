#ifndef REGISTER_HPP
#define REGISTER_HPP

/*
  REGISTER.hpp (Provisório - JP)
  Principais pontos:
  - Implementação inline para facilitar inclusão direta e testes.
  - reverse_read() realiza um byte-swap (endianness reverse). Isso é útil
    quando você precisa inspecionar a palavra no formato little-endian vs
    big-endian ou quando está montando/desmontando bytes.
  - Uso de tipos fixos (<cstdint>) para portabilidade.
  - Comentários explicando comportamento, custos e alternativas.
*/

#include <cstdint>
#include <cstring>   // memcpy (opcional)
#include <algorithm> // std::reverse (não usado para performance)
#include <cassert>   // assert (checagem em debug)

struct REGISTER {
    // Armazena o valor do registrador (palavra de 32 bits)
    // Inicializado em 0 por convenção.
    uint32_t value;

    // Construtor: zera o registrador por padrão
    REGISTER() : value(0u) {}

    // Escreve um novo valor no registrador.
    // Nota: não há proteção contra escrita em registrador zero (R0) aqui;
    // se quiser essa proteção (como MIPS), trate-a em um nível superior
    // (ex.: no banco de registradores).
    inline void write(uint32_t new_value) {
        value = new_value;
    }

    // read()
    // - Retorna o valor atual do registrador.
    // - Marcado [[nodiscard]] para evitar que código acidentalmente
    //   ignore o valor retornado (útil em operações críticas).
    // - Const: não modifica o registrador.
    [[nodiscard]] inline uint32_t read() const {
        return value;
    }

    // reverse_read()
    // - Retorna o valor com os bytes invertidos (endianness swap).
    //   Ex.: 0x11223344 -> 0x44332211
    // - Útil para:
    //     * desmontar palavras em bytes na ordem "humana"
    //     * exportar dados para dispositivos com endianness diferente
    //     * depuração / verificação de formatos de instrução
    // - Implementação eficiente usando bitwise (sem dependência de memcpy).
    [[nodiscard]] inline uint32_t reverse_read() const {
        // byte swap manual (operações bitwise são portáveis e rápidas)
        uint32_t x = value;
        return ((x & 0x000000FFu) << 24) |
               ((x & 0x0000FF00u) << 8)  |
               ((x & 0x00FF0000u) >> 8)  |
               ((x & 0xFF000000u) >> 24);
    }

    // Opcionais (comentados) — adições úteis que alguém pode querer:
    //
    // inline void reset() { value = 0u; }
    //   - Reseta o registrador para zero.
    //
    // inline void write_if_not_zero(uint32_t new_value) {
    //   // Em arquiteturas MIPS geralmente, R0 é sempre zero: você pode proteger aqui:
    //   if (this != &r0_register) value = new_value;
    // }
};

#endif // REGISTER_HPP
