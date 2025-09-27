#include "SecondMemory.hpp"


int main(){

    SecondMemory disco;

    std :: cout << "Tamanho da memoria : " << disco.getSize() << std :: endl;

    disco.write(0, 10);
    disco.write(1, 20);
    disco.write(2, 30);
    disco.write(3, 40);
    disco.write(4, 50);
    
    std :: cout << "Lendo dados da memoria secundaria : " << std :: endl;
    for(int i = 0; i < 5; i++){
        std :: cout << "Endereco : " << i << " Dado : " << disco.read(i) << std :: endl;
    }
    
    int dataToSearch = 30;
    size_t address = disco.searchInMemory(dataToSearch);
    if (address != (size_t)-1) {
        std::cout << "Dado " << dataToSearch << " encontrado no endereço: " << address << std::endl;
    } else {
        std::cout << "Dado " << dataToSearch << " não encontrado na memória." << std::endl;
    }

    int dado = disco.read(address);
    std::cout << "Lendo dado do endereco encontrado: " << dado << std::endl;    

    auto dado_invalido = 999;

    address = disco.searchInMemory(dado_invalido);
    if (address != (size_t)-1) {
        std::cout << "Dado " << dado_invalido << " encontrado no endereço: " << address << std::endl;
    } else {
        std::cout << "Dado " << dado_invalido << " não encontrado na memória." << std::endl;
    }

    std:: cout << "\n Testando acesso de endereco invalido : \n" << std :: endl;
    try{
        disco.read(disco.getSize() + 1);
    } catch (const std::out_of_range& e) {
        std::cout << "Exceção capturada: " << e.what() << std::endl;
    }

    return 0;
}