#include "SecondMemory.hpp"

SecondMemory::SecondMemory() {

    std :: cout <<"Construtor SecondMemory : Inicializando memoria secundaria" << std :: endl;
    init(LINHAS * COLUNAS);
}

SecondMemory::~SecondMemory() {
    std :: cout <<"Destrutor SecondMemory : Liberando memoria secundaria" << std :: endl;
}


void SecondMemory::init(size_t size) {
    if (size > LINHAS * COLUNAS) {
        std::cerr << "Tamanho excede a capacidade da memória." << std::endl;
        return;
    }
    for (size_t i = 0; i < LINHAS; ++i) {
        for (size_t j = 0; j < COLUNAS; ++j) {
            memory[i][j] = -1; // Inicializa todos os elementos com -1
        }
    }
}

void SecondMemory::write(size_t address, int data) {

    if (address >= LINHAS * COLUNAS) {
        std::cerr << "Endereço fora do limite da memória." << std::endl;
        return;
    }

    size_t row = address / COLUNAS; 
    size_t col = address % COLUNAS; 
    memory[row][col] = data;
}

int SecondMemory::read(size_t address) {

    if (address >= LINHAS * COLUNAS) {
        std::cerr << "Endereço fora do limite da memória." << std::endl;
        return -1; // Retorna -1 para indicar erro
    }

    size_t linhas = address / COLUNAS;
    size_t colunas = address % COLUNAS;
    return memory[linhas][colunas];
}


size_t SecondMemory::searchInMemory(int data) const {
    for (size_t i = 0; i < LINHAS; ++i) {
        for (size_t j = 0; j < COLUNAS; ++j) {
            if (memory[i][j] == data) {
                return (size_t)(i * COLUNAS + j); // Retorna o endereço linear
            }
        }
    }
    return -1; // Retorna -1 se o dado não for encontrado
}


int SecondMemory::getSize() const {
    return LINHAS * COLUNAS;
}