#ifndef __HASHREGISTER_HPP
#define __HASHREGISTER_HPP

/*
  HashRegister.hpp (Provisório - JP)
  Principais pontos:
  Pequena utilidade para mapear códigos binários (5 bits, como "00000")
  para nomes simbólicos de registradores ("zero", "ra", "at", ...).
  - Uso de std::map para mapeamento (ordenação e comportamento previsível);
  - Implementação de getRegister() que retorna "UNKNOWN" se não achar a chave;
  - Sobrecarga getRegister(int idx) que aceita índice 0..31 e converte para
    o código binário correspondente ("00000" .. "11111");
  - Método estático helper binFromIndex para conversão índice -> string binária.
  Registradores de Uso Específico
    pc: Program Counter
    mar: Memory Address Register
    cr: Cause Register
    epc: Exception Program Counter
    sr: Status Register
    hi, lo: Armazenam resultados de operações de 32 bits
    ir: Instruction Register
  Registradores de Uso Geral
    zero: Sempre contém 0
    at: Reservado para o assembler
    v0, v1: Contém os valores de retorno de funções
    a0 - a3: Contém os argumentos necessários para as chamadas de função
    t0 - t9: Registradores temporários
    s0 - s7: Registradores salvos
    k0, k1: Reservados para o sistema operacional
    gp, sp, fp, ra: Global Pointer, Stack Pointer, Frame Pointer, Return Address
*/

#include <map>
#include <string>
#include <array>
#include <sstream>
#include <iomanip>

namespace hw { // encapsula para evitar poluição global
    using std::map;
    using std::string;
    using std::array;

    struct Map {
        // mapa que associa código binário (5 bits) -> nome simbólico do registrador
        map<string, string> mp;

        // Construtor: preenche o mapa uma vez na criação da instância
        Map() {
            mp["00000"] = "zero";
            mp["00001"] = "ra";
            mp["00010"] = "at";
            mp["00011"] = "v0";
            mp["00100"] = "v1";
            mp["00101"] = "a0";
            mp["00110"] = "a1";
            mp["00111"] = "a2";
            mp["01000"] = "a3";
            mp["01001"] = "t0";
            mp["01010"] = "t1";
            mp["01011"] = "t2";
            mp["01100"] = "t3";
            mp["01101"] = "t4";
            mp["01110"] = "t5";
            mp["01111"] = "t6";
            mp["10000"] = "t7";
            mp["10001"] = "s0";
            mp["10010"] = "s1";
            mp["10011"] = "s2";
            mp["10100"] = "s3";
            mp["10101"] = "s4";
            mp["10110"] = "s5";
            mp["10111"] = "s6";
            mp["11000"] = "s7";
            mp["11001"] = "t8";
            mp["11010"] = "t9";
            mp["11011"] = "k0";
            mp["11100"] = "k1";
            mp["11101"] = "gp";
            mp["11110"] = "sp";
            mp["11111"] = "fp";
        }

        // Converte um índice inteiro (0..31) para a string binária de 5 bits.
        // Ex.: 0 -> "00000", 5 -> "00101".
        static string binFromIndex(int idx) {
            if (idx < 0 || idx > 31) return std::string(); // retorna vazio em erro
            // usa std::bitset ou stringstream; bitset é simples, mas para evitar includes extras:
            std::string s(5, '0');
            for (int i = 4; i >= 0; --i) {
                s[i] = (idx & 1) ? '1' : '0';
                idx >>= 1;
            }
            return s;
        }

        // Retorna o nome do registrador a partir do código binário (ex: "00001").
        // Se não existir, retorna "UNKNOWN".
        string getRegister(const string &codeofregister) const {
            auto it = mp.find(codeofregister);
            if (it == mp.end()) {
                return "UNKNOWN";
            }
            return it->second;
        }

        // Sobrecarga: aceita índice inteiro (0..31). Em caso de índice inválido,
        // retorna "UNKNOWN".
        string getRegister(int index) const {
            string key = binFromIndex(index);
            if (key.empty()) return "UNKNOWN";
            return getRegister(key);
        }
    };

} // namespace hw

#endif // __HASHREGISTER_HPP
