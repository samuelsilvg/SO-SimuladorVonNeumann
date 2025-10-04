#include "cpu/REGISTER_BANK.hpp" 

using namespace hw;
using namespace std;

// Função para testar as funcionalidades básicas de escrita e leitura
void basicFunctionalityTest_Bank(){
    cout << "\n=== Basic Functionality Test (REGISTER_BANK) ===\n";

    REGISTER_BANK banco;

    // Teste 1: Estado Inicial. Todos devem começar em 0.
    cout << "Verificando estado inicial (todos os regs devem ser 0)... ";
    if (banco.readRegister("t5") == 0 && banco.readRegister("s2") == 0 && banco.readRegister("pc") == 0){
        cout << "OK.\n";
    } else{
        cout << "FALHA.\n";
    }

    // Teste 2: Escrita e Leitura
    cout << "Escrevendo valor 42 no registrador 's1'...\n";
    banco.writeRegister("s1", 42);
    uint32_t valor_s1 = banco.readRegister("s1");
    cout << "Valor lido de 's1': " << valor_s1 << " (esperado: 42)\n";
    if (valor_s1 == 42){
        cout << "  -> SUCESSO: O valor foi escrito e lido corretamente.\n";
    } else{
        cout << "  -> FALHA: O valor lido e diferente do que foi escrito.\n";
    }
}

// Função para testar as regras especiais e o tratamento de erros
void rulesAndErrorHandlingTest_Bank(){
    cout << "\n=== Rules & Error Handling Test (REGISTER_BANK) ===\n";

    REGISTER_BANK banco;

    // Teste 1: Proteção do Registrador 'zero'
    cout << "Testando a protecao do registrador 'zero'...\n";
    cout << "  Tentando escrever 999 em 'zero'...\n";
    banco.writeRegister("zero", 999);
    uint32_t valor_zero = banco.readRegister("zero");
    cout << "  Valor lido de 'zero': " << valor_zero << " (esperado: 0)\n";
    if (valor_zero == 0){
        cout << "  -> SUCESSO: O registrador 'zero' permaneceu 0.\n";
    } else{
        cout << "  -> FALHA: O registrador 'zero' foi modificado!\n";
    }

    // Teste 2: Acesso a registrador inválido (leitura)
    cout << "Testando leitura de registrador invalido ('sp2')...\n";
    try{
        banco.readRegister("sp2");
        cout << "  -> FALHA: O programa deveria ter lancado um erro.\n";
    } catch (const runtime_error& e){
        cout << "  -> SUCESSO: O programa lancou um erro como esperado.\n";
        cout << "     Mensagem: " << e.what() << endl;
    }
}

// Função para testar o método reset e a impressão
void utilsTest_Bank(){
    cout << "\n=== Utilities Test (reset & print) ===\n";
    REGISTER_BANK banco;

    banco.writeRegister("t0", 10);
    banco.writeRegister("a0", 20);
    banco.writeRegister("ra", 30);

    cout << "Estado dos registradores ANTES do reset:\n";
    banco.print_registers();

    // Testa o reset
    cout << "\nExecutando reset()...\n";
    banco.reset();

    cout << "Estado dos registradores DEPOIS do reset:\n";
    banco.print_registers();

    cout << "Verificando se 't0' e 0 apos o reset: " << banco.readRegister("t0") << " (esperado: 0)\n";
    if(banco.readRegister("t0") == 0 && banco.readRegister("ra") == 0){
        cout << "  -> SUCESSO: O reset funcionou corretamente.\n";
    } else{
        cout << "  -> FALHA: O reset nao zerou os registradores.\n";
    }
}


int main(){
    cout << "===============================================\n";
    cout << "=== Iniciando Teste Unitario: REGISTER_BANK ===\n";
    cout << "===============================================\n";

    try{
        basicFunctionalityTest_Bank();
        rulesAndErrorHandlingTest_Bank();
        utilsTest_Bank();

        cout << "\n=== Todos os testes do REGISTER_BANK passaram com sucesso! ===\n";

    } catch (const exception& e){
        cout << "\n!!! Teste falhou com uma excecao: " << e.what() << " !!!\n";
        return 1;
    }

    return 0;
}
