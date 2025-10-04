/*
Sujeito a alterações - Eduardo

- REGISTER_BANK(): Ele preenche os mapas que associam os nomes dos registradores (ex: "t0") 
às suas funções de leitura e escrita. É aqui que a mágica do acesso por nome acontece.

- readRegister(): Lê um registrador usando o nome como string. Lança um erro se o
nome for inválido.

- writeRegister(): Escreve em um registrador usando o nome. A proteção do registrador "zero" é garantida aqui.

- reset(): Zera todos os registradores. Serve para limpar o estado da CPU entre processos.

- print_registers(): Função de ajuda para debug. Imprime o valor de todos os registradores de forma organizada na tela.
*/

#include "REGISTER_BANK.hpp" 

namespace hw{

REGISTER_BANK::REGISTER_BANK(){
    // --- Cada entrada no mapa chama o método read() do registrador correspondente ---
    acessoLeituraRegistradores = {
        {"pc",   [this](){ return this->pc.read(); }}, {"mar",  [this](){ return this->mar.read(); }},
        {"cr",   [this](){ return this->cr.read(); }},  {"epc",  [this](){ return this->epc.read(); }},
        {"sr",   [this](){ return this->sr.read(); }},  {"hi",   [this](){ return this->hi.read(); }},
        {"lo",   [this](){ return this->lo.read(); }},  {"ir",   [this](){ return this->ir.read(); }},
        {"zero", [this](){ return this->zero.read();}},{"at",   [this](){ return this->at.read(); }},
        {"v0",   [this](){ return this->v0.read(); }},  {"v1",   [this](){ return this->v1.read(); }},
        {"a0",   [this](){ return this->a0.read(); }},  {"a1",   [this](){ return this->a1.read(); }},
        {"a2",   [this](){ return this->a2.read(); }},  {"a3",   [this](){ return this->a3.read(); }},
        {"t0",   [this](){ return this->t0.read(); }},  {"t1",   [this](){ return this->t1.read(); }},
        {"t2",   [this](){ return this->t2.read(); }},  {"t3",   [this](){ return this->t3.read(); }},
        {"t4",   [this](){ return this->t4.read(); }},  {"t5",   [this](){ return this->t5.read(); }},
        {"t6",   [this](){ return this->t6.read(); }},  {"t7",   [this](){ return this->t7.read(); }},
        {"t8",   [this](){ return this->t8.read(); }},  {"t9",   [this](){ return this->t9.read(); }},
        {"s0",   [this](){ return this->s0.read(); }},  {"s1",   [this](){ return this->s1.read(); }},
        {"s2",   [this](){ return this->s2.read(); }},  {"s3",   [this](){ return this->s3.read(); }},
        {"s4",   [this](){ return this->s4.read(); }},  {"s5",   [this](){ return this->s5.read(); }},
        {"s6",   [this](){ return this->s6.read(); }},  {"s7",   [this](){ return this->s7.read(); }},
        {"k0",   [this](){ return this->k0.read(); }},  {"k1",   [this](){ return this->k1.read(); }},
        {"gp",   [this](){ return this->gp.read(); }},  {"sp",   [this](){ return this->sp.read(); }},
        {"fp",   [this](){ return this->fp.read(); }},  {"ra",   [this](){ return this->ra.read(); }}
    };

    // --- Cada entrada no mapa chama o método write() do registrador correspondente ---
    acessoEscritaRegistradores = {
        {"pc",   [this](uint32_t v){ this->pc.write(v); }}, {"mar",  [this](uint32_t v){ this->mar.write(v); }},
        {"cr",   [this](uint32_t v){ this->cr.write(v); }},  {"epc",  [this](uint32_t v){ this->epc.write(v); }},
        {"sr",   [this](uint32_t v){ this->sr.write(v); }},  {"hi",   [this](uint32_t v){ this->hi.write(v); }},
        {"lo",   [this](uint32_t v){ this->lo.write(v); }},  {"ir",   [this](uint32_t v){ this->ir.write(v); }},
        {"at",   [this](uint32_t v){ this->at.write(v); }},
        {"v0",   [this](uint32_t v){ this->v0.write(v); }},  {"v1",   [this](uint32_t v){ this->v1.write(v); }},
        {"a0",   [this](uint32_t v){ this->a0.write(v); }},  {"a1",   [this](uint32_t v){ this->a1.write(v); }},
        {"a2",   [this](uint32_t v){ this->a2.write(v); }},  {"a3",   [this](uint32_t v){ this->a3.write(v); }},
        {"t0",   [this](uint32_t v){ this->t0.write(v); }},  {"t1",   [this](uint32_t v){ this->t1.write(v); }},
        {"t2",   [this](uint32_t v){ this->t2.write(v); }},  {"t3",   [this](uint32_t v){ this->t3.write(v); }},
        {"t4",   [this](uint32_t v){ this->t4.write(v); }},  {"t5",   [this](uint32_t v){ this->t5.write(v); }},
        {"t6",   [this](uint32_t v){ this->t6.write(v); }},  {"t7",   [this](uint32_t v){ this->t7.write(v); }},
        {"t8",   [this](uint32_t v){ this->t8.write(v); }},  {"t9",   [this](uint32_t v){ this->t9.write(v); }},
        {"s0",   [this](uint32_t v){ this->s0.write(v); }},  {"s1",   [this](uint32_t v){ this->s1.write(v); }},
        {"s2",   [this](uint32_t v){ this->s2.write(v); }},  {"s3",   [this](uint32_t v){ this->s3.write(v); }},
        {"s4",   [this](uint32_t v){ this->s4.write(v); }},  {"s5",   [this](uint32_t v){ this->s5.write(v); }},
        {"s6",   [this](uint32_t v){ this->s6.write(v); }},  {"s7",   [this](uint32_t v){ this->s7.write(v); }},
        {"k0",   [this](uint32_t v){ this->k0.write(v); }},  {"k1",   [this](uint32_t v){ this->k1.write(v); }},
        {"gp",   [this](uint32_t v){ this->gp.write(v); }},  {"sp",   [this](uint32_t v){ this->sp.write(v); }},
        {"fp",   [this](uint32_t v){ this->fp.write(v); }},  {"ra",   [this](uint32_t v){ this->ra.write(v); }},
        
        // Proteção do registrador ZERO.
        {"zero", [](uint32_t /*v*/){ /* Nao faz nada */ }}
    };
}

uint32_t REGISTER_BANK::readRegister(const string &name) const{
    auto it = acessoLeituraRegistradores.find(name);

    if (it == acessoLeituraRegistradores.end()){
        throw runtime_error("Erro: Tentativa de ler um registrador que nao existe: " + name);
    }

    return it->second();
}

void REGISTER_BANK::writeRegister(const string &name, uint32_t value){
    auto it = acessoEscritaRegistradores.find(name);

    if (it == acessoEscritaRegistradores.end()){
        throw runtime_error("Erro: Tentativa de escrever em um registrador que nao existe: " + name);
    }

    it->second(value);
}

void REGISTER_BANK::reset(){
    for (auto const& [name, func] : acessoEscritaRegistradores){
        func(0);
    }
}

void REGISTER_BANK::print_registers() const{
    auto printPair = [](const string &name, uint32_t value){
        cout << left << setw(6) << name << ": 0x"
             << hex << setw(8) << setfill('0') << value
             << dec << setfill(' ') << "  (Decimal: " << static_cast<int32_t>(value) << ")\n";
    };

    cout << "========================================\n";
    cout << "====== Estado do Banco de Registradores ======\n";
    cout << "========================================\n";
    printPair("pc", pc.read()); printPair("ir", ir.read());
    printPair("mar", mar.read()); printPair("sr", sr.read());
    printPair("hi", hi.read()); printPair("lo", lo.read());
    cout << "----------------------------------------\n";
    printPair("zero", zero.read()); printPair("at", at.read());
    printPair("v0", v0.read());   printPair("v1", v1.read());
    printPair("a0", a0.read());   printPair("a1", a1.read());
    printPair("a2", a2.read());   printPair("a3", a3.read());
    cout << "----------------------------------------\n";
    printPair("t0", t0.read());   printPair("t1", t1.read());
    printPair("t2", t2.read());   printPair("t3", t3.read());
    printPair("t4", t4.read());   printPair("t5", t5.read());
    printPair("t6", t6.read());   printPair("t7", t7.read());
    printPair("t8", t8.read());   printPair("t9", t9.read());
    cout << "----------------------------------------\n";
    printPair("s0", s0.read());   printPair("s1", s1.read());
    printPair("s2", s2.read());   printPair("s3", s3.read());
    printPair("s4", s4.read());   printPair("s5", s5.read());
    printPair("s6", s6.read());   printPair("s7", s7.read());
    cout << "----------------------------------------\n";
    printPair("gp", gp.read());   printPair("sp", sp.read());
    printPair("fp", fp.read());   printPair("ra", ra.read());
    printPair("k0", k0.read());   printPair("k1", k1.read());
    cout << "========================================\n";
}
string REGISTER_BANK::get_registers_as_string() const {
    std::ostringstream ss;
    auto printPair = [&ss](const std::string &name, uint32_t value){
        ss << std::left << std::setw(6) << name << ": 0x"
           << std::hex << std::setw(8) << std::setfill('0') << value
           << std::dec << std::setfill(' ') << "  (Decimal: "
           << static_cast<int32_t>(value) << ")\n";
    };

    ss << "========================================\n";
    ss << "====== Estado do Banco de Registradores ======\n";
    ss << "========================================\n";
    printPair("pc", pc.read()); printPair("ir", ir.read());
    printPair("mar", mar.read()); printPair("sr", sr.read());
    printPair("hi", hi.read()); printPair("lo", lo.read());
    ss << "----------------------------------------\n";
    printPair("zero", zero.read()); printPair("at", at.read());
    printPair("v0", v0.read());   printPair("v1", v1.read());
    printPair("a0", a0.read());   printPair("a1", a1.read());
    printPair("a2", a2.read());   printPair("a3", a3.read());
    ss << "----------------------------------------\n";
    printPair("t0", t0.read());   printPair("t1", t1.read());
    printPair("t2", t2.read());   printPair("t3", t3.read());
    printPair("t4", t4.read());   printPair("t5", t5.read());
    printPair("t6", t6.read());   printPair("t7", t7.read());
    printPair("t8", t8.read());   printPair("t9", t9.read());
    ss << "----------------------------------------\n";
    printPair("s0", s0.read());   printPair("s1", s1.read());
    printPair("s2", s2.read());   printPair("s3", s3.read());
    printPair("s4", s4.read());   printPair("s5", s5.read());
    printPair("s6", s6.read());   printPair("s7", s7.read());
    ss << "----------------------------------------\n";
    printPair("gp", gp.read());   printPair("sp", sp.read());
    printPair("fp", fp.read());   printPair("ra", ra.read());
    printPair("k0", k0.read());   printPair("k1", k1.read());
    ss << "========================================\n";

    return ss.str();
}



} 

