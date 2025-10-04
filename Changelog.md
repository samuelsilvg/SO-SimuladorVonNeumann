# Histórico de Mudanças do Simulador Von Neumann

Este documento resume as principais alterações e integrações realizadas para transformar os módulos individuais em um simulador funcional, com escalonamento de processos e hierarquia de memória completa.

## 1. Sistema de Build: Migração de `Makefile` para `CMake`

O sistema de compilação foi migrado de um `Makefile` manual para `CMake`, trazendo modernidade e robustez ao projeto.

- **Automação:** O `CMakeLists.txt` agora gerencia automaticamente a compilação de todos os arquivos `.cpp` necessários para cada executável, eliminando a necessidade de atualizar listas de fontes manualmente.
- **Portabilidade:** O projeto agora pode ser compilado em diferentes sistemas operacionais (Linux, Windows, macOS) sem alterações no `CMakeLists.txt`.
- **Comandos Amigáveis:** Foram criados alvos personalizados para imitar a conveniência do `Makefile` original:
  - `make run`: Executa o simulador principal.
  - `make test-all`: Roda todos os testes.
  - `make check`: Faz uma verificação rápida de todos os componentes.
  - `make ajuda`: Exibe um menu de ajuda.

## 2. Correção da Hierarquia e Políticas de Memória

A lógica de gerenciamento de memória foi completamente refatorada para seguir os requisitos da especificação (`Pratica0.pdf`).

- **Política de Cache FIFO:** A política de substituição da cache, que antes era baseada em timestamp (LRU), foi substituída por uma implementação de **FIFO (First-In, First-Out)**, utilizando uma `std::queue` para garantir a ordem de remoção correta.
- **Implementação de Write-Back:** A política de escrita foi finalizada. Agora, quando um bloco de dados "sujo" (`isDirty = true`) é removido da cache, o `MemoryManager` garante que ele seja escrito de volta na memória principal antes da substituição.
- **Simulação de Acesso Lento:** A `SECONDARY_MEMORY` foi alterada para simular o alto custo de acesso de um disco. As operações de leitura e escrita agora realizam uma varredura na estrutura de dados em vez de um acesso direto, introduzindo a latência necessária.

## 3. Criação do Escalonador e Integração dos Módulos

O `main.cpp` foi criado para atuar como o núcleo do sistema operacional, orquestrando todos os módulos.

- **Escalonador Round-Robin:** Foi implementado um laço principal de escalonamento que gerencia uma fila de processos prontos (`ready_queue`). Ele funciona da seguinte forma:
  1. Retira o primeiro processo da fila e o coloca em estado de `Running`.
  2. Executa o processo na CPU (função `Core`) por uma fatia de tempo (quantum).
  3. Após a execução, avalia o estado do processo:
     - Se o quantum expirou, o processo volta para o final da fila (`Ready`).
     - Se o processo terminou, suas métricas são impressas e ele é removido do ciclo.
     - Se o processo solicitou I/O, ele é movido para a lista de bloqueados.

- **Integração do `IOManager`:** O escalonador agora faz a ponte entre a CPU e o `IOManager`. Quando a `CONTROL_UNIT` define o estado de um processo como `Blocked`, o laço principal detecta essa mudança e chama a função `ioManager.registerProcessWaitingForIO()`, entregando o processo para ser gerenciado pela thread de I/O. O escalonador também verifica periodicamente se algum processo bloqueado já foi liberado pelo `IOManager` para retorná-lo à fila de prontos.

- **Integração do `parser_json`:** O parser de programas foi modificado para interagir com o `MemoryManager` em vez de escrever diretamente na `MAIN_MEMORY`. Isso garante que o carregamento do programa na inicialização respeite toda a hierarquia de memória, incluindo a cache.


## 4. Correções Gerais de Compilação

- Resolvido o erro de cópia do `PCB` (que continha membros `std::atomic` não-copiáveis) ao passar a gerenciar os processos através de ponteiros (`std::unique_ptr<PCB>`).
- Corrigido o erro de "tipo incompleto" do `IORequest` ao mover sua definição para um header acessível (`IOManager.hpp`).
- Solucionados erros de *linker* (`undefined reference`) ao configurar corretamente todas as fontes necessárias para cada executável no `CMakeLists.txt`.
- Corrigidas inconsistências e erros de digitação no código, como `hw::BANK` vs `hw::REGISTER_BANK` e `id` vs `pid`.