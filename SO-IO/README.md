# Módulo de Gerenciamento de I/O (IOManager)

## Estrutura dos Arquivos

* `IOManager.h`: Arquivo de cabeçalho da classe `IOManager`. Define a interface pública e os membros privados da classe.
* `IOManager.cpp`: Arquivo de implementação da classe `IOManager`. Contém toda a lógica de funcionamento do gerenciador.
* `shared_structs.h`: Define estruturas de dados e enums (`PCB`, `IORequest`, `State`) que são compartilhados entre o `IOManager` e outros módulos do sistema (como a CPU e o Escalonador).
* `main.cpp`: **Arquivo de simulação e exemplo de uso.** Ele cria um ambiente simulado com processos e um escalonador para demonstrar como um sistema operacional interagiria com o `IOManager`.

## Arquitetura do Projeto

O projeto é dividido em duas partes principais:

1.  **O Módulo `IOManager`**: É o núcleo deste trabalho. Sua responsabilidade é receber, enfileirar e processar solicitações de I/O (como escrita em disco ou interação com o usuário). Ele é completamente independente do resto do sistema e se comunica através de uma única função pública.

2.  **O Ambiente de Simulação (`main.cpp`)**: Este código **não faz parte** do módulo `IOManager`. Ele atua como um "cliente" ou "driver" que utiliza o gerenciador. Ele simula:
    * A criação de Processos (PCBs).
    * Um escalonador de CPU (Round-Robin simples).
    * A execução de processos na CPU.
    * A geração de requisições de I/O que são enviadas para o `IOManager`.

## Como Usar o Módulo `IOManager`

Para integrar este gerenciador em outro sistema (como o módulo de CPU), são necessários 3 passos:

1.  **Instanciar o Gerenciador**: Crie uma única instância do `IOManager` no escopo principal do sistema.
    ```cpp
    #include "IOManager.h"

    IOManager io_manager;
    ```

2.  **Criar uma Requisição**: Quando um processo solicitar uma operação de I/O, crie e preencha uma estrutura `IORequest`.
    ```cpp
    #include "shared_structs.h"
    #include <memory>

    auto request = std::make_unique<IORequest>();
    request->process = ponteiro_para_o_pcb_do_processo;
    request->operation = "write_result";
    request->msg = "Dados a serem escritos.";
    request->cost_cycles = std::chrono::milliseconds(500); // Custo da operação
    ```

3.  **Enviar a Requisição**: Use o método público `addRequest` para entregar a requisição ao gerenciador. O processo que fez a solicitação deve ser movido para o estado `Blocked`.
    ```cpp
    processo_solicitante->state = State::Blocked;
    io_manager.addRequest(std::move(request));
    ```

## Métodos Principais do `IOManager.cpp`

### 1. `void IOManager::addRequest(std::unique_ptr<IORequest> request)`

Este é o **ponto de entrada** do `IOManager`. É a única função pública usada para interagir com o gerenciador.

* **Responsabilidade**: Adicionar de forma segura uma nova requisição de I/O à fila de processamento.
* **Funcionamento**:
    1.  Ela recebe um `std::unique_ptr<IORequest>`, o que significa que o `IOManager` assume a "posse" da requisição. Isso evita vazamentos de memória.
    2.  Para garantir que a fila não seja modificada por múltiplos threads ao mesmo tempo (condição de corrida), a função utiliza um `std::lock_guard<std::mutex>`. Este "cadeado" bloqueia o acesso à fila enquanto a nova requisição está sendo inserida.
    3.  A requisição é movida para o final do vetor `requests`. Assim que o `lock_guard` sai de escopo, o mutex é liberado automaticamente.

### 2. `void IOManager::managerLoop()`

É uma função privada que executa em um loop infinito dentro de sua própria thread, gerenciando e processando as requisições.

* **Responsabilidade**: Monitorar continuamente a fila de requisições, processar a próxima requisição disponível e atualizar o estado do processo solicitante ao final.
* **Funcionamento**:
    1.  A função opera dentro de um laço `while (!shutdown_flag)`, que a mantém ativa durante toda a execução do programa.
    2.  A cada iteração, ela também usa um `std::lock_guard` para verificar com segurança se a fila (`requests`) não está vazia.
    3.  Se houver uma requisição, ela é retirada da frente da fila.
    4.  **Processamento da Requisição**:
        * Simula o custo em tempo da operação usando `std::this_thread::sleep_for`.
        * Escreve logs informativos no console (`std::cout`).
        * Grava os resultados em dois arquivos de saída: `result.dat` (formato legível) e `output.dat` (formato CSV).
        * Ao final, **libera o processo** que estava bloqueado, alterando seu estado de volta para `State::Ready`, permitindo que ele volte a ser escalonado pela CPU.
    5.  Se a fila estiver vazia, a thread "dorme" por um curto período para evitar consumo desnecessário de CPU.

## Saídas Geradas

* `result.dat`: Um arquivo de log em formato de texto, que descreve cada operação de I/O concluída.
* `output.dat`: Um arquivo de dados em formato CSV (`id,operação,duração`) para fácil importação e análise por outras ferramentas.

## Como Compilar e Executar

```bash
make clean
make
./io
