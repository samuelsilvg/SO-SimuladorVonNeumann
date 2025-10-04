# Módulo de Gerenciamento de I/O (IOManager)

## Estrutura dos Arquivos

* `IOManager.h`: Arquivo de cabeçalho da classe `IOManager`. Define a interface pública e os membros privados.
* `IOManager.cpp`: Arquivo de implementação da classe `IOManager`. Contém toda a lógica de funcionamento do gerenciador.
* `shared_structs.h`: Define estruturas de dados e enums (`PCB`, `IORequest`, `State`) que são compartilhados entre o `IOManager` e outros módulos.
* `main.cpp`: **Arquivo de simulação e exemplo de uso.** Ele cria um ambiente com processos e um escalonador para demonstrar a interação com o `IOManager`.

## Arquitetura do Projeto

O projeto é dividido em duas partes principais:

1.  **O Módulo `IOManager`**: É o núcleo deste trabalho. Sua responsabilidade agora é dupla:
    * **Simular Dispositivos**: Ele simula hardware (como impressora e disco) que, de forma independente, solicitam operações de I/O.
    * **Gerenciar Processos**: Ele mantém uma fila de processos que estão bloqueados esperando por I/O e os atribui aos dispositivos que se tornam ativos. Ele gera as requisições de I/O internamente.

2.  **O Ambiente de Simulação (`main.cpp`)**: Este código **não faz parte** do módulo `IOManager`. Ele atua como um "cliente" que utiliza o gerenciador, simulando:
    * A criação de Processos (PCBs).
    * Um escalonador de CPU (Round-Robin simples).
    * A decisão de um processo de solicitar uma operação de I/O, momento em que ele se "registra" no `IOManager` e fica bloqueado.

## Como Usar o Módulo `IOManager`

Para integrar este gerenciador em outro sistema, o processo foi simplificado:

1.  **Instanciar o Gerenciador**: Crie uma única instância do `IOManager` no escopo principal do sistema.
    ```cpp
    #include "IOManager.h"

    IOManager io_manager;
    ```

2.  **Registrar um Processo para I/O**: Quando um processo em execução na CPU decidir que precisa de uma operação de I/O, ele deve ser movido para o estado `Blocked` e, em seguida, registrar-se no gerenciador usando o método `registerProcessWaitingForIO`.
    ```cpp
    #include "shared_structs.h"

    // Supondo que 'processo_solicitante' é um ponteiro para o PCB
    processo_solicitante->state = State::Blocked;
    io_manager.registerProcessWaitingForIO(processo_solicitante);
    ```
    A partir daqui, o `IOManager` assume a responsabilidade de eventualmente atender a este processo quando um dispositivo ficar disponível.

## Métodos Principais do `IOManager.cpp`

### 1. `void IOManager::registerProcessWaitingForIO(PCB* process)`

Este é o **novo ponto de entrada** do `IOManager`. É a única função pública usada por sistemas externos para interagir com o gerenciador.

* **Responsabilidade**: Adicionar de forma segura um processo que entrou em estado `Blocked` a uma lista de espera interna.
* **Funcionamento**:
    1.  Recebe um ponteiro para o PCB do processo que precisa de I/O.
    2.  Utiliza um `std::lock_guard<std::mutex>` para bloquear o acesso à lista `waiting_processes` e evitar condições de corrida.
    3.  Adiciona o processo à lista de espera.

### 2. `void IOManager::managerLoop()`

É uma função privada que executa em um loop infinito dentro de sua própria thread, representando o ciclo de vida do gerenciador. Sua lógica foi expandida e agora opera em três etapas principais a cada iteração:

* **Responsabilidade**: Simular dispositivos, combinar processos em espera com dispositivos ativos, criar requisições de I/O e processá-las.
* **Funcionamento**:
    1.  **Etapa 1: Simulação de Dispositivos**
        * De forma aleatória, o loop pode alterar o estado de um dos dispositivos (ex: `printer_requesting`) de `false` para `true`. Isso simula um periférico que agora precisa de serviço, representando o "estado 1" que foi solicitado.

    2.  **Etapa 2: Verificação e Criação de Requisições**
        * O gerenciador verifica duas condições simultaneamente: se há algum dispositivo com estado `true` E se há algum processo na `waiting_processes`.
        * Se ambas forem verdadeiras, ele "combina" os dois:
            * Pega o primeiro processo da fila de espera.
            * Cria uma estrutura `IORequest` específica para o dispositivo ativo (ex: `operation = "print_job"`).
            * **Atribui um custo aleatório de 1 a 3** à requisição.
            * Muda o estado do dispositivo de volta para `false` (ocupado ou atendido).
            * Adiciona a requisição recém-criada à fila de processamento interna.

    3.  **Etapa 3: Processamento da Requisição**
        * Se a fila de processamento não estiver vazia, a primeira requisição é retirada.
        * Simula o custo em tempo da operação usando `std::this_thread::sleep_for`.
        * Grava logs no console e nos arquivos `result.dat` e `output.dat`.
        * Ao final, **libera o processo** que estava bloqueado, alterando seu estado de volta para `State::Ready`, permitindo que ele volte a ser escalonado pela CPU.

## Saídas Geradas

* `result.dat`: Um arquivo de log em formato de texto, que descreve cada operação de I/O concluída.
* `output.dat`: Um arquivo de dados em formato CSV (`id,operação,duração`) para fácil importação e análise.

## Como Compilar e Executar

```bash
make clean
make
./io
