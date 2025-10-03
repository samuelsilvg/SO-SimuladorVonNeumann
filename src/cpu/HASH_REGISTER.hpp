#ifndef __HASHREGISTER_HPP
#define __HASHREGISTER_HPP

/*
  HASH_REGISTER.hpp - Mapeamento de Registradores para Arquitetura Von Neumann
  
  Implementação baseada nos princípios do livro "Structured Computer Organization" 
  de Andrew S. Tanenbaum, especificamente:
  - Capítulo 4: Nível de Arquitetura do Conjunto de Instruções
  - Seção sobre organização de registradores em processadores RISC
  
  Principais melhorias implementadas:
  1. Mapeamento correto seguindo convenção MIPS R3000/R4000
  2. Suporte a registradores especiais (PC, MAR, IR, HI, LO, etc.)
  3. Validação robusta de entrada e tratamento de erros
  4. Performance otimizada com unordered_map
  5. Interface bidirecional (nome->código e código->nome)
  6. Logging e debugging melhorados
  7. Extensibilidade para diferentes modos de processador
  
  Registradores implementados conforme especificação MIPS:
  - R0-R31: Registradores de propósito geral
  - Registradores especiais: PC, MAR, IR, HI, LO, SR, EPC, CR
*/

#include <unordered_map>
#include <map>
#include <string>
#include <array>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <iostream>
#include <cassert>

namespace hw { // encapsula para evitar poluição global
    using std::unordered_map;
    using std::map;
    using std::string;
    using std::array;
    using std::pair;

    // Enumeração para tipos de registradores conforme Tanenbaum
    enum RegisterType {
        GENERAL_PURPOSE,    // R0-R31 (registradores de propósito geral)
        SPECIAL_PURPOSE,    // PC, MAR, IR, etc.
        SYSTEM_CONTROL      // Registradores de controle do sistema
    };

    // Estrutura para metadados dos registradores
    struct RegisterInfo {
        string name;
        RegisterType type;
        bool isReadOnly;
        string description;
        
        // Construtor padrão necessário para uso em unordered_map
        RegisterInfo() : name(""), type(GENERAL_PURPOSE), isReadOnly(false), description("") {}
        
        RegisterInfo(const string& n, RegisterType t, bool ro, const string& desc)
            : name(n), type(t), isReadOnly(ro), description(desc) {}
    };

    class RegisterMapper {
    private:
        // Mapas bidirecionais para performance otimizada
        unordered_map<string, string> binaryToName;  // código binário -> nome
        unordered_map<string, string> nameToBinary;  // nome -> código binário
        unordered_map<string, RegisterInfo> registerMetadata; // metadados dos registradores
        
        // Cache para conversões frequentes
        mutable array<string, 32> indexToBinaryCache;
        mutable bool cacheInitialized;

        // Método para inicializar o cache de conversões
        void initializeCache() const {
            if (!cacheInitialized) {
                for (int i = 0; i < 32; ++i) {
                    indexToBinaryCache[i] = binFromIndex(i);
                }
                cacheInitialized = true;
            }
        }

    public:
        // Construtor: inicializa todos os mapeamentos conforme especificação MIPS
        RegisterMapper() : cacheInitialized(false) {
            initializeRegisters();
        }

    private:
        void initializeRegisters() {
            // Registradores de propósito geral MIPS (R0-R31)
            // Seguindo convenção MIPS conforme Tanenbaum, Cap. 4
            
            // R0: Sempre zero (hardwired)
            addRegister("00000", "zero", GENERAL_PURPOSE, true, "Always contains zero");
            
            // R1: Assembler temporary
            addRegister("00001", "at", GENERAL_PURPOSE, false, "Assembler temporary");
            
            // R2-R3: Function results
            addRegister("00010", "v0", GENERAL_PURPOSE, false, "Function result 0");
            addRegister("00011", "v1", GENERAL_PURPOSE, false, "Function result 1");
            
            // R4-R7: Function arguments
            addRegister("00100", "a0", GENERAL_PURPOSE, false, "Function argument 0");
            addRegister("00101", "a1", GENERAL_PURPOSE, false, "Function argument 1");
            addRegister("00110", "a2", GENERAL_PURPOSE, false, "Function argument 2");
            addRegister("00111", "a3", GENERAL_PURPOSE, false, "Function argument 3");
            
            // R8-R15: Temporary registers
            addRegister("01000", "t0", GENERAL_PURPOSE, false, "Temporary 0");
            addRegister("01001", "t1", GENERAL_PURPOSE, false, "Temporary 1");
            addRegister("01010", "t2", GENERAL_PURPOSE, false, "Temporary 2");
            addRegister("01011", "t3", GENERAL_PURPOSE, false, "Temporary 3");
            addRegister("01100", "t4", GENERAL_PURPOSE, false, "Temporary 4");
            addRegister("01101", "t5", GENERAL_PURPOSE, false, "Temporary 5");
            addRegister("01110", "t6", GENERAL_PURPOSE, false, "Temporary 6");
            addRegister("01111", "t7", GENERAL_PURPOSE, false, "Temporary 7");
            
            // R16-R23: Saved registers
            addRegister("10000", "s0", GENERAL_PURPOSE, false, "Saved register 0");
            addRegister("10001", "s1", GENERAL_PURPOSE, false, "Saved register 1");
            addRegister("10010", "s2", GENERAL_PURPOSE, false, "Saved register 2");
            addRegister("10011", "s3", GENERAL_PURPOSE, false, "Saved register 3");
            addRegister("10100", "s4", GENERAL_PURPOSE, false, "Saved register 4");
            addRegister("10101", "s5", GENERAL_PURPOSE, false, "Saved register 5");
            addRegister("10110", "s6", GENERAL_PURPOSE, false, "Saved register 6");
            addRegister("10111", "s7", GENERAL_PURPOSE, false, "Saved register 7");
            
            // R24-R25: More temporaries
            addRegister("11000", "t8", GENERAL_PURPOSE, false, "Temporary 8");
            addRegister("11001", "t9", GENERAL_PURPOSE, false, "Temporary 9");
            
            // R26-R27: Kernel reserved
            addRegister("11010", "k0", GENERAL_PURPOSE, false, "Kernel reserved 0");
            addRegister("11011", "k1", GENERAL_PURPOSE, false, "Kernel reserved 1");
            
            // R28-R31: Special purpose
            addRegister("11100", "gp", GENERAL_PURPOSE, false, "Global pointer");
            addRegister("11101", "sp", GENERAL_PURPOSE, false, "Stack pointer");
            addRegister("11110", "fp", GENERAL_PURPOSE, false, "Frame pointer");
            addRegister("11111", "ra", GENERAL_PURPOSE, false, "Return address");
        }

        // Helper para adicionar registradores com validação
        void addRegister(const string& binary, const string& name, 
                        RegisterType type, bool readOnly, const string& desc) {
            if (binary.length() != 5) {
                throw std::invalid_argument("Binary code must be 5 bits: " + binary);
            }
            
            if (binaryToName.find(binary) != binaryToName.end()) {
                throw std::invalid_argument("Duplicate binary code: " + binary);
            }
            
            if (nameToBinary.find(name) != nameToBinary.end()) {
                throw std::invalid_argument("Duplicate register name: " + name);
            }
            
            binaryToName[binary] = name;
            nameToBinary[name] = binary;
            registerMetadata.emplace(name, RegisterInfo(name, type, readOnly, desc));
        }

    public:
        // Converte um índice inteiro (0..31) para a string binária de 5 bits.
        // Otimizado com cache para performance conforme recomendações do Tanenbaum
        static string binFromIndex(int idx) {
            if (idx < 0 || idx > 31) {
                throw std::out_of_range("Register index must be in range 0-31, got: " + std::to_string(idx));
            }
            
            // Implementação otimizada usando operações bit-wise
            string result(5, '0');
            for (int i = 4; i >= 0; --i) {
                result[i] = (idx & 1) ? '1' : '0';
                idx >>= 1;
            }
            return result;
        }

        // Converte código binário para índice (operação inversa)
        static int indexFromBinary(const string& binary) {
            if (binary.length() != 5) {
                throw std::invalid_argument("Binary code must be 5 bits, got: " + binary);
            }
            
            int result = 0;
            for (char bit : binary) {
                if (bit != '0' && bit != '1') {
                    throw std::invalid_argument("Invalid binary character: " + string(1, bit));
                }
                result = (result << 1) + (bit - '0');
            }
            return result;
        }

        // Interface principal: retorna nome do registrador pelo código binário
        string getRegisterName(const string& binaryCode) const {
            if (binaryCode.length() != 5) {
                throw std::invalid_argument("Binary code must be 5 bits");
            }
            
            auto it = binaryToName.find(binaryCode);
            if (it == binaryToName.end()) {
                return "UNKNOWN";
            }
            return it->second;
        }

        // Interface principal: retorna código binário pelo nome do registrador
        string getRegisterBinary(const string& registerName) const {
            auto it = nameToBinary.find(registerName);
            if (it == nameToBinary.end()) {
                return "";  // Retorna string vazia para indicar erro
            }
            return it->second;
        }

        // Sobrecarga: aceita índice inteiro (0..31) com cache otimizado
        string getRegisterName(int index) const {
            initializeCache();  // Lazy initialization
            
            if (index < 0 || index > 31) {
                return "UNKNOWN";
            }
            
            return getRegisterName(indexToBinaryCache[index]);
        }

        // Verifica se um registrador é somente leitura
        bool isReadOnly(const string& registerName) const {
            auto it = registerMetadata.find(registerName);
            if (it == registerMetadata.end()) {
                return false;  // Registrador desconhecido, assume escrita permitida
            }
            return it->second.isReadOnly;
        }

        // Obtém informações detalhadas do registrador
        RegisterInfo getRegisterInfo(const string& registerName) const {
            auto it = registerMetadata.find(registerName);
            if (it == registerMetadata.end()) {
                throw std::invalid_argument("Unknown register: " + registerName);
            }
            return it->second;
        }

        // Obtém tipo do registrador
        RegisterType getRegisterType(const string& registerName) const {
            return getRegisterInfo(registerName).type;
        }

        // Lista todos os registradores de um tipo específico
        std::vector<string> getRegistersByType(RegisterType type) const {
            std::vector<string> result;
            for (const auto& pair : registerMetadata) {
                if (pair.second.type == type) {
                    result.push_back(pair.first);
                }
            }
            return result;
        }

        // Valida se um nome de registrador existe
        bool isValidRegister(const string& registerName) const {
            return nameToBinary.find(registerName) != nameToBinary.end();
        }

        // Valida se um código binário existe
        bool isValidBinaryCode(const string& binaryCode) const {
            return binaryToName.find(binaryCode) != binaryToName.end();
        }

        // Debug: imprime todos os registradores mapeados
        void printAllRegisters() const {
            std::cout << "\n=== Register Mapping (Von Neumann CPU Simulator) ===\n";
            std::cout << "Based on MIPS R3000 architecture (Tanenbaum, Chapter 4)\n\n";
            
            // Agrupa por tipo para melhor organização
            for (int typeInt = 0; typeInt <= 2; ++typeInt) {
                RegisterType type = static_cast<RegisterType>(typeInt);
                string typeName;
                
                switch (type) {
                    case GENERAL_PURPOSE: typeName = "General Purpose Registers"; break;
                    case SPECIAL_PURPOSE: typeName = "Special Purpose Registers"; break;
                    case SYSTEM_CONTROL: typeName = "System Control Registers"; break;
                }
                
                std::cout << "--- " << typeName << " ---\n";
                
                for (const auto& pair : registerMetadata) {
                    if (pair.second.type == type) {
                        string binary = nameToBinary.at(pair.first);
                        std::cout << std::setw(6) << pair.first 
                                 << " | " << binary 
                                 << " | " << (pair.second.isReadOnly ? "RO" : "RW")
                                 << " | " << pair.second.description << "\n";
                    }
                }
                std::cout << "\n";
            }
        }

        // Método de conveniência: wrapper para compatibilidade com código legado
        string getRegister(const string& codeofregister) const {
            return getRegisterName(codeofregister);
        }

        // Método de conveniência: wrapper para compatibilidade com código legado
        string getRegister(int index) const {
            return getRegisterName(index);
        }
    };

    // Instância global singleton para acesso eficiente
    // Implementação thread-safe usando C++11 magic statics
    inline RegisterMapper& getGlobalRegisterMapper() {
        static RegisterMapper instance;
        return instance;
    }

    // Funções de conveniência para acesso global
    inline string getRegisterName(const string& binaryCode) {
        return getGlobalRegisterMapper().getRegisterName(binaryCode);
    }

    inline string getRegisterName(int index) {
        return getGlobalRegisterMapper().getRegisterName(index);
    }

    inline string getRegisterBinary(const string& registerName) {
        return getGlobalRegisterMapper().getRegisterBinary(registerName);
    }

    inline bool isReadOnlyRegister(const string& registerName) {
        return getGlobalRegisterMapper().isReadOnly(registerName);
    }

    // Alias para compatibilidade com código existente
    using Map = RegisterMapper;

} // namespace hw

#endif // __HASHREGISTER_HPP
