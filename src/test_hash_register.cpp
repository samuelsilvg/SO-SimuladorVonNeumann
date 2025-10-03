#include <iostream>
#include <chrono>
#include <vector>
#include "cpu/HASH_REGISTER.hpp"

using namespace hw;
using namespace std;

// Função para testar performance
void performanceTest() {
    cout << "\n=== Performance Test ===\n";
    
    auto& mapper = getGlobalRegisterMapper();
    const int iterations = 100000;
    
    // Teste de performance: conversão índice -> nome
    auto start = chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        for (int reg = 0; reg < 32; ++reg) {
            volatile string name = mapper.getRegisterName(reg);
        }
    }
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    
    cout << "Index->Name conversion (" << iterations << " * 32 ops): " 
         << duration.count() << " μs\n";
    
    // Teste de performance: conversão nome -> binário
    vector<string> registerNames = {"zero", "ra", "sp", "t0", "s0", "a0", "v0", "gp"};
    
    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        for (const auto& name : registerNames) {
            volatile string binary = mapper.getRegisterBinary(name);
        }
    }
    end = chrono::high_resolution_clock::now();
    duration = chrono::duration_cast<chrono::microseconds>(end - start);
    
    cout << "Name->Binary conversion (" << iterations << " * " << registerNames.size() 
         << " ops): " << duration.count() << " μs\n";
}

// Função para testar funcionalidades básicas
void basicFunctionalityTest() {
    cout << "\n=== Basic Functionality Test ===\n";
    
    auto& mapper = getGlobalRegisterMapper();
    
    // Teste de conversões básicas
    cout << "Register 0: " << mapper.getRegisterName(0) << " (should be 'zero')\n";
    cout << "Register 31: " << mapper.getRegisterName(31) << " (should be 'ra')\n";
    cout << "Register 'sp': " << mapper.getRegisterBinary("sp") << " (should be '11101')\n";
    cout << "Register 't0': " << mapper.getRegisterBinary("t0") << " (should be '01000')\n";
    
    // Teste de validação
    cout << "\nValidation tests:\n";
    cout << "Is 'zero' valid? " << (mapper.isValidRegister("zero") ? "Yes" : "No") << "\n";
    cout << "Is 'invalid' valid? " << (mapper.isValidRegister("invalid") ? "Yes" : "No") << "\n";
    cout << "Is 'zero' read-only? " << (mapper.isReadOnly("zero") ? "Yes" : "No") << "\n";
    cout << "Is 't0' read-only? " << (mapper.isReadOnly("t0") ? "Yes" : "No") << "\n";
    
    // Teste de informações detalhadas
    try {
        auto info = mapper.getRegisterInfo("zero");
        cout << "\nRegister 'zero' info:\n";
        cout << "  Description: " << info.description << "\n";
        cout << "  Type: " << (info.type == GENERAL_PURPOSE ? "General Purpose" : "Other") << "\n";
        cout << "  Read-only: " << (info.isReadOnly ? "Yes" : "No") << "\n";
    } catch (const exception& e) {
        cout << "Error getting register info: " << e.what() << "\n";
    }
}

// Função para testar tratamento de erros
void errorHandlingTest() {
    cout << "\n=== Error Handling Test ===\n";
    
    auto& mapper = getGlobalRegisterMapper();
    
    // Teste de índice inválido
    cout << "Invalid index (-1): " << mapper.getRegisterName(-1) << " (should be 'UNKNOWN')\n";
    cout << "Invalid index (32): " << mapper.getRegisterName(32) << " (should be 'UNKNOWN')\n";
    
    // Teste de nome inválido
    cout << "Invalid name 'xyz': '" << mapper.getRegisterBinary("xyz") << "' (should be empty)\n";
    
    // Teste de código binário inválido - captura exceção
    try {
        cout << "Invalid binary '11111111': " << mapper.getRegisterName("11111111") << " (should throw exception)\n";
    } catch (const invalid_argument& e) {
        cout << "Invalid binary correctly threw exception: " << e.what() << "\n";
    }
    
    // Teste de exceções
    try {
        RegisterMapper::binFromIndex(-1);
        cout << "ERROR: Should have thrown exception for invalid index\n";
    } catch (const out_of_range& e) {
        cout << "Correctly caught exception for invalid index: " << e.what() << "\n";
    }
    
    try {
        RegisterMapper::indexFromBinary("invalid");
        cout << "ERROR: Should have thrown exception for invalid binary\n";
    } catch (const invalid_argument& e) {
        cout << "Correctly caught exception for invalid binary: " << e.what() << "\n";
    }
}

// Função para demonstrar filtros por tipo
void typeFilterTest() {
    cout << "\n=== Type Filter Test ===\n";
    
    auto& mapper = getGlobalRegisterMapper();
    
    auto gpRegisters = mapper.getRegistersByType(GENERAL_PURPOSE);
    cout << "General Purpose Registers (" << gpRegisters.size() << " total):\n";
    for (size_t i = 0; i < min(size_t(8), gpRegisters.size()); ++i) {
        cout << "  " << gpRegisters[i] << " (" << mapper.getRegisterBinary(gpRegisters[i]) << ")\n";
    }
    if (gpRegisters.size() > 8) {
        cout << "  ... and " << (gpRegisters.size() - 8) << " more\n";
    }
}

int main() {
    cout << "=== HASH_REGISTER Improved Implementation Test ===\n";
    cout << "Based on Tanenbaum's 'Structured Computer Organization'\n";
    cout << "Chapter 4: The Instruction Set Architecture Level\n";
    
    try {
        basicFunctionalityTest();
        errorHandlingTest();
        typeFilterTest();
        performanceTest();
        
        // Demonstrar o mapeamento completo
        auto& mapper = getGlobalRegisterMapper();
        mapper.printAllRegisters();
        
        cout << "\n=== Test Completed Successfully ===\n";
        
    } catch (const exception& e) {
        cout << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
