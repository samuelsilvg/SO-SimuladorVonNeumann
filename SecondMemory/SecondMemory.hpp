#ifndef SECOND_MEMORY_HPP
#define SECOND_MEMORY_HPP
#define  LINHAS 128
#define  COLUNAS 128

#include <bits/stdc++.h>


class SecondMemory {
public:
    SecondMemory(); 
    ~SecondMemory();

    // inicializa a memória
    void init(size_t size);
    // escreve um dado 
    void write(size_t address, int data);
    //le um dado a partir do endereço
    int read(size_t address);
    // busca um dado e retorna o endereço
    size_t searchInMemory(int data) const;
    // retorna o tamanho da memória
    int getSize() const;


private:

    int memory[LINHAS][COLUNAS]; 
  
};
    





#endif 