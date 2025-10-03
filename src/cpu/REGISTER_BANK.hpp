/*
Sujeito a alterações - Eduardo

O banco de registradores é, na teoria, a memória mais rápida da CPU. Ele funciona
como uma "mesa de trabalho" para o processador, guardando os dados que estão
sendo usados no momento, como o resultado de uma soma ou o endereço da próxima
instrução.

Na prática, aqui no nosso código, o REGISTER_BANK é uma classe que agrupa todos
os registradores do MIPS como objetos individuais. A ideia é que, em vez
de acessar um registrador por um número (como o registrador 16), a Control Unit
pode simplesmente pedir pelo nome ("s0"), usando os mapas que a gente criou.
Isso deixa o código do resto do grupo muito mais fácil de ler e entender.

Este arquivo .hpp é a "interface" da minha parte. Ele só diz o que a classe
faz e quais funções ela tem. 
*/

#ifndef REGISTER_BANK_HPP
#define REGISTER_BANK_HPP

#include "REGISTER.hpp"

#include <cstdint>
#include <string>

#include <unordered_map>
#include <functional>

#include <stdexcept>
#include <iostream>
#include <iomanip>


using namespace std;

// Namespace para o nosso hardware simulado. Serve para evitar que os nomes das nossas classes (como REGISTER_BANK) entrem em conflito com outras bibliotecas.
namespace hw{

    // Junta todos os registradores da CPU e fornece a interface de acesso por nome que a Control_Unit vai usar.
    class REGISTER_BANK{
    public:
        // --- Registradores de uso específico ---
        REGISTER pc, mar, cr, epc, sr, hi, lo, ir;

        // --- Registradores de uso geral (seguindo a convenção MIPS) ---
        REGISTER zero, at;
        REGISTER v0, v1;
        REGISTER a0, a1, a2, a3;
        REGISTER t0, t1, t2, t3, t4, t5, t6, t7, t8, t9;
        REGISTER s0, s1, s2, s3, s4, s5, s6, s7;
        REGISTER k0, k1;
        REGISTER gp, sp, fp, ra;

        // --- Mapas de acesso por nome (a interface principal para a Control_Unit) ---
        unordered_map<string, function<uint32_t()>> acessoLeituraRegistradores;
        unordered_map<string, function<void(uint32_t)>> acessoEscritaRegistradores;

        // Construtor: Declarado aqui, implementado no .cpp
        REGISTER_BANK();

        // Leitura segura por nome.
        uint32_t readRegister(const string &name) const;

        // Escrita segura por nome.
        void writeRegister(const string &name, uint32_t value);

        // Zera todos os registradores.
        void reset();

        // Imprime o estado de todos os registradores na tela.
        void print_registers() const;
    };

} 

#endif 

