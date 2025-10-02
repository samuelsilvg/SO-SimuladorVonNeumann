#pragma once
#include <unordered_map>

class MainMemory {
private:
    // endereço (byte) -> word (32 bits)
    std::unordered_map<int,int> mem_;
public:
    void WriteMem(int addr, int value);  // grava word 32b em 'addr'
    int  ReadMem (int addr) const;       // lê word 32b (0 se não existir)
    void Clear();                        // limpa toda memória
    void DumpMem() const;                // imprime em ordem de endereço
};
