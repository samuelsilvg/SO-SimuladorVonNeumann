#ifndef REGISTER_BANK_HPP
#define REGISTER_BANK_HPP

/*
  REGISTER_BANK.hpp (Provisório - JP)

  Banco de registradores com acesso por nome (string) usando mapas de funções.
  - Protege o registrador "zero" contra escrita (sempre 0).
  - Fornece mapas para leitura/escrita (acessoLeituraRegistradores /
    acessoEscritaRegistradores) para compatibilidade com o código existente.
  - Implementa helpers seguros: readRegister(name), writeRegister(name,val),
    snapshot() e reset().
  - print_registers() imprime os registradores em formato legível para debug.

  Observações:
  - Usa a struct REGISTER (REGISTER.hpp) para representar cada registrador.
  - O design prioriza clareza e segurança (evita escrever diretamente em .value).
*/

#include <iomanip>
#include <iostream>
#include <string>
#include <map>
#include <unordered_map>
#include <functional>
#include <vector>
#include <stdexcept>

#include "REGISTER.hpp"

namespace hw { // namespace para evitar poluição global
using std::string;
using std::map;
using std::unordered_map;
using std::function;
using std::uint32_t;
using std::cout;
using std::endl;

/*
  REGISTER_BANK
  - Campos: registradores especiais + registradores gerais (seguindo convenções MIPS)
  - acessoLeituraRegistradores: map<string, function<uint32_t()>> para ler por nome
  - acessoEscritaRegistradores: map<string, function<void(uint32_t)>> para escrever
*/
struct REGISTER_BANK {
    // registradores de uso específico
    REGISTER pc;   // program counter
    REGISTER mar;  // memory address register
    REGISTER cr;   // cause register
    REGISTER epc;  // exception program counter
    REGISTER sr;   // status register

    REGISTER hi; // para resultado alto (multiplicação/divisão 64-bit)
    REGISTER lo; // para resultado baixo
    REGISTER ir; // instruction register

    // registradores de uso geral
    REGISTER zero; // sempre 0 (proteção aplicada)
    REGISTER at;
    REGISTER v0, v1;
    REGISTER a0, a1, a2, a3;
    REGISTER t0, t1, t2, t3, t4, t5, t6, t7, t8, t9;
    REGISTER s0, s1, s2, s3, s4, s5, s6, s7;
    REGISTER k0, k1;
    REGISTER gp, sp, fp, ra;

    // mapas de acesso por nome (compatível com o que você usou)
    // uso unordered_map para lookup amortizado O(1)
    unordered_map<string, function<uint32_t()>> acessoLeituraRegistradores;
    unordered_map<string, function<void(uint32_t)>> acessoEscritaRegistradores;

    // Construtor: inicializa mapas de leitura/escrita (usa lambdas que chamam REGISTER::read/write)
    REGISTER_BANK() {
        // --- Leitura: cada entrada retorna REGISTER::read()
        acessoLeituraRegistradores["hi"] = [this]() { return this->hi.read(); };
        acessoLeituraRegistradores["mar"] = [this]() { return this->mar.read(); };
        acessoLeituraRegistradores["pc"] = [this]() { return this->pc.read(); };
        acessoLeituraRegistradores["cr"] = [this]() { return this->cr.read(); };
        acessoLeituraRegistradores["epc"] = [this]() { return this->epc.read(); };
        acessoLeituraRegistradores["sr"] = [this]() { return this->sr.read(); };
        acessoLeituraRegistradores["lo"] = [this]() { return this->lo.read(); };
        acessoLeituraRegistradores["ir"] = [this]() { return this->ir.read(); };

        acessoLeituraRegistradores["zero"] = [this]() { return this->zero.read(); };
        acessoLeituraRegistradores["at"]   = [this]() { return this->at.read(); };
        acessoLeituraRegistradores["v0"]   = [this]() { return this->v0.read(); };
        acessoLeituraRegistradores["v1"]   = [this]() { return this->v1.read(); };
        acessoLeituraRegistradores["a0"]   = [this]() { return this->a0.read(); };
        acessoLeituraRegistradores["a1"]   = [this]() { return this->a1.read(); };
        acessoLeituraRegistradores["a2"]   = [this]() { return this->a2.read(); };
        acessoLeituraRegistradores["a3"]   = [this]() { return this->a3.read(); };

        acessoLeituraRegistradores["t0"] = [this]() { return this->t0.read(); };
        acessoLeituraRegistradores["t1"] = [this]() { return this->t1.read(); };
        acessoLeituraRegistradores["t2"] = [this]() { return this->t2.read(); };
        acessoLeituraRegistradores["t3"] = [this]() { return this->t3.read(); };
        acessoLeituraRegistradores["t4"] = [this]() { return this->t4.read(); };
        acessoLeituraRegistradores["t5"] = [this]() { return this->t5.read(); };
        acessoLeituraRegistradores["t6"] = [this]() { return this->t6.read(); };
        acessoLeituraRegistradores["t7"] = [this]() { return this->t7.read(); };
        acessoLeituraRegistradores["t8"] = [this]() { return this->t8.read(); };
        acessoLeituraRegistradores["t9"] = [this]() { return this->t9.read(); };

        acessoLeituraRegistradores["s0"] = [this]() { return this->s0.read(); };
        acessoLeituraRegistradores["s1"] = [this]() { return this->s1.read(); };
        acessoLeituraRegistradores["s2"] = [this]() { return this->s2.read(); };
        acessoLeituraRegistradores["s3"] = [this]() { return this->s3.read(); };
        acessoLeituraRegistradores["s4"] = [this]() { return this->s4.read(); };
        acessoLeituraRegistradores["s5"] = [this]() { return this->s5.read(); };
        acessoLeituraRegistradores["s6"] = [this]() { return this->s6.read(); };
        acessoLeituraRegistradores["s7"] = [this]() { return this->s7.read(); };

        acessoLeituraRegistradores["k0"] = [this]() { return this->k0.read(); };
        acessoLeituraRegistradores["k1"] = [this]() { return this->k1.read(); };
        acessoLeituraRegistradores["gp"] = [this]() { return this->gp.read(); };
        acessoLeituraRegistradores["sp"] = [this]() { return this->sp.read(); };
        acessoLeituraRegistradores["fp"] = [this]() { return this->fp.read(); };
        acessoLeituraRegistradores["ra"] = [this]() { return this->ra.read(); };

        // --- Escrita: usa REGISTER::write()
        // Protegemos "zero" contra escrita: lambda vazia (ignora writes)
        acessoEscritaRegistradores["hi"] = [this](uint32_t val) { this->hi.write(val); };
        acessoEscritaRegistradores["mar"] = [this](uint32_t val) { this->mar.write(val); };
        acessoEscritaRegistradores["pc"] = [this](uint32_t val) { this->pc.write(val); };
        acessoEscritaRegistradores["cr"] = [this](uint32_t val) { this->cr.write(val); };
        acessoEscritaRegistradores["epc"] = [this](uint32_t val) { this->epc.write(val); };
        acessoEscritaRegistradores["sr"] = [this](uint32_t val) { this->sr.write(val); };
        acessoEscritaRegistradores["lo"] = [this](uint32_t val) { this->lo.write(val); };
        acessoEscritaRegistradores["ir"] = [this](uint32_t val) { this->ir.write(val); };

        // zero: protegido (não altera o registrador)
        acessoEscritaRegistradores["zero"] = [this](uint32_t /*val*/) {
            // intentionally empty to keep zero == 0
        };

        acessoEscritaRegistradores["at"] = [this](uint32_t val) { this->at.write(val); };
        acessoEscritaRegistradores["v0"] = [this](uint32_t val) { this->v0.write(val); };
        acessoEscritaRegistradores["v1"] = [this](uint32_t val) { this->v1.write(val); };
        acessoEscritaRegistradores["a0"] = [this](uint32_t val) { this->a0.write(val); };
        acessoEscritaRegistradores["a1"] = [this](uint32_t val) { this->a1.write(val); };
        acessoEscritaRegistradores["a2"] = [this](uint32_t val) { this->a2.write(val); };
        acessoEscritaRegistradores["a3"] = [this](uint32_t val) { this->a3.write(val); };

        acessoEscritaRegistradores["t0"] = [this](uint32_t val) { this->t0.write(val); };
        acessoEscritaRegistradores["t1"] = [this](uint32_t val) { this->t1.write(val); };
        acessoEscritaRegistradores["t2"] = [this](uint32_t val) { this->t2.write(val); };
        acessoEscritaRegistradores["t3"] = [this](uint32_t val) { this->t3.write(val); };
        acessoEscritaRegistradores["t4"] = [this](uint32_t val) { this->t4.write(val); };
        acessoEscritaRegistradores["t5"] = [this](uint32_t val) { this->t5.write(val); };
        acessoEscritaRegistradores["t6"] = [this](uint32_t val) { this->t6.write(val); };
        acessoEscritaRegistradores["t7"] = [this](uint32_t val) { this->t7.write(val); };
        acessoEscritaRegistradores["t8"] = [this](uint32_t val) { this->t8.write(val); };
        acessoEscritaRegistradores["t9"] = [this](uint32_t val) { this->t9.write(val); };

        acessoEscritaRegistradores["s0"] = [this](uint32_t val) { this->s0.write(val); };
        acessoEscritaRegistradores["s1"] = [this](uint32_t val) { this->s1.write(val); };
        acessoEscritaRegistradores["s2"] = [this](uint32_t val) { this->s2.write(val); };
        acessoEscritaRegistradores["s3"] = [this](uint32_t val) { this->s3.write(val); };
        acessoEscritaRegistradores["s4"] = [this](uint32_t val) { this->s4.write(val); };
        acessoEscritaRegistradores["s5"] = [this](uint32_t val) { this->s5.write(val); };
        acessoEscritaRegistradores["s6"] = [this](uint32_t val) { this->s6.write(val); };
        acessoEscritaRegistradores["s7"] = [this](uint32_t val) { this->s7.write(val); };

        acessoEscritaRegistradores["k0"] = [this](uint32_t val) { this->k0.write(val); };
        acessoEscritaRegistradores["k1"] = [this](uint32_t val) { this->k1.write(val); };
        acessoEscritaRegistradores["gp"] = [this](uint32_t val) { this->gp.write(val); };
        acessoEscritaRegistradores["sp"] = [this](uint32_t val) { this->sp.write(val); };
        acessoEscritaRegistradores["fp"] = [this](uint32_t val) { this->fp.write(val); };
        acessoEscritaRegistradores["ra"] = [this](uint32_t val) { this->ra.write(val); };

        // Garantir que zero comece em 0
        zero.write(0u);
    }

    // Leitura segura por nome. Se nome inválido -> lança runtime_error.
    uint32_t readRegister(const string &name) const {
        auto it = acessoLeituraRegistradores.find(name);
        if (it == acessoLeituraRegistradores.end()) {
            throw std::runtime_error("readRegister: nome de registrador inválido: " + name);
        }
        return it->second();
    }

    // Escrita segura por nome. Se nome inválido -> lança runtime_error.
    // Proteção para "zero" está implementada nas lambdas (ignora escrita).
    void writeRegister(const string &name, uint32_t value) {
        auto it = acessoEscritaRegistradores.find(name);
        if (it == acessoEscritaRegistradores.end()) {
            throw std::runtime_error("writeRegister: nome de registrador inválido: " + name);
        }
        it->second(value);
    }

    // Retorna um snapshot (cópia) do estado dos registradores com pares (nome, valor).
    // Útil para debug / logging.
    std::map<string, uint32_t> snapshot() const {
        std::map<string, uint32_t> snap;
        for (const auto &p : acessoLeituraRegistradores) {
            try {
                snap[p.first] = p.second();
            } catch (...) {
                snap[p.first] = 0u; // fallback em caso de erro
            }
        }
        return snap;
    }

    // Reseta todos os registradores para 0 (exceto "zero" que já é 0).
    // Use com cuidado (pode afetar PC, SP, etc).
    void reset() {
        // registradores especiais
        hi.write(0); lo.write(0); ir.write(0);
        pc.write(0); mar.write(0); cr.write(0); epc.write(0); sr.write(0);
        // gerais
        at.write(0); v0.write(0); v1.write(0);
        a0.write(0); a1.write(0); a2.write(0); a3.write(0);
        t0.write(0); t1.write(0); t2.write(0); t3.write(0); t4.write(0);
        t5.write(0); t6.write(0); t7.write(0); t8.write(0); t9.write(0);
        s0.write(0); s1.write(0); s2.write(0); s3.write(0); s4.write(0);
        s5.write(0); s6.write(0); s7.write(0);
        k0.write(0); k1.write(0);
        gp.write(0); sp.write(0); fp.write(0); ra.write(0);
        // zero permanece 0 (não precisa escrever)
        zero.write(0);
    }

    // Imprime registradores de forma organizada (útil para debug)
    void print_registers() const {
        // grupo de registradores por categoria para uma saída limpa
        auto printPair = [](const string &name, uint32_t value) {
            cout << std::left << std::setw(6) << name << ": 0x"
                 << std::hex << std::setw(8) << std::setfill('0') << value
                 << std::dec << std::setfill(' ') << "  (" << value << ")\n";
        };

        cout << "=== Register Bank ===\n";
        // especiais
        printPair("pc", pc.read());
        printPair("mar", mar.read());
        printPair("ir", ir.read());
        printPair("hi", hi.read());
        printPair("lo", lo.read());
        printPair("cr", cr.read());
        printPair("epc", epc.read());
        printPair("sr", sr.read());
        cout << "\nGeneral purpose registers:\n";

        // prints em linhas organizadas: v0 v1 a0 a1 a2 a3
        printPair("zero", zero.read());
        printPair("at", at.read());
        printPair("v0", v0.read());
        printPair("v1", v1.read());
        printPair("a0", a0.read());
        printPair("a1", a1.read());
        printPair("a2", a2.read());
        printPair("a3", a3.read());
        cout << "\nTemporaries:\n";
        printPair("t0", t0.read()); printPair("t1", t1.read()); printPair("t2", t2.read()); printPair("t3", t3.read());
        printPair("t4", t4.read()); printPair("t5", t5.read()); printPair("t6", t6.read()); printPair("t7", t7.read());
        printPair("t8", t8.read()); printPair("t9", t9.read());
        cout << "\nSaved:\n";
        printPair("s0", s0.read()); printPair("s1", s1.read()); printPair("s2", s2.read()); printPair("s3", s3.read());
        printPair("s4", s4.read()); printPair("s5", s5.read()); printPair("s6", s6.read()); printPair("s7", s7.read());
        cout << "\nOS/Stack:\n";
        printPair("k0", k0.read()); printPair("k1", k1.read()); printPair("gp", gp.read()); printPair("sp", sp.read());
        printPair("fp", fp.read()); printPair("ra", ra.read());
        cout << "======================\n";
    }
};

} // namespace hw

#endif // REGISTER_BANK_HPP
