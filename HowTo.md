Claro\! Aqui está o guia completo em formato Markdown, pronto para você copiar e colar diretamente em um arquivo `README.md` no seu repositório.

-----

# Simulador de Arquitetura Von Neumann e Pipeline MIPS

Este projeto é um simulador em C++ que implementa uma arquitetura de Von Neumann, incluindo uma CPU com pipeline MIPS de 5 estágios, uma hierarquia de memória (Cache, RAM, Disco) e um gerenciador de I/O. O sistema é gerenciado por um escalonador Round-Robin que executa processos carregados a partir de arquivos JSON.

## 📋 Pré-requisitos

Para compilar e executar este projeto, você precisará ter os seguintes softwares instalados:

  * `g++` (com suporte a C++17)
  * `CMake` (versão 3.10 ou superior)
  * `make`

## ⚙️ Como Compilar o Projeto

O projeto utiliza `CMake` para gerar os arquivos de compilação. O processo é simples e deve ser feito a partir do terminal.

1.  **Abra o terminal** na pasta raiz do projeto.

2.  **Crie e acesse um diretório de build:** É uma boa prática manter os arquivos de compilação separados do código-fonte.

    ```bash
    mkdir build
    cd build
    ```

3.  **Execute o CMake:** Este comando irá configurar o projeto e gerar o `Makefile` dentro da pasta `build`.

    ```bash
    cmake ..
    ```

4.  **Compile tudo:** Use o comando `make` para compilar o simulador principal e todos os testes.

    ```bash
    make
    ```

    Após a compilação, todos os executáveis estarão dentro da pasta `build`.

## 🚀 Como Executar o Simulador

Para rodar a simulação principal, você pode usar o executável `simulador` ou o alvo personalizado `run`.

#### Opção 1: Executando diretamente

Certifique-se de que você está dentro da pasta `build`.

```bash
./simulador
```

#### Opção 2: Usando o alvo `run`

Este comando compila o projeto (se necessário) e o executa em seguida.

```bash
# Estando dentro da pasta 'build'
make run
```

**Arquivos Necessários:** O simulador precisa dos arquivos `process1.json` e `tasks.json` para rodar. O sistema de build está configurado para copiá-los automaticamente para a pasta `build` durante a compilação.

## 🧪 Como Rodar os Testes

O projeto inclui vários testes para validar o funcionamento de cada módulo. Você pode executá-los usando os alvos `make` correspondentes de dentro da pasta `build`.

  * **Rodar todos os testes de uma vez:**

    ```bash
    make test-all
    ```

  * **Verificação rápida (Passou/Falhou):**

    ```bash
    make check
    ```

  * **Executar testes individuais:**

      * **Teste da ULA:** `make test_ula`
      * **Teste do Mapeador de Registradores:** `make test_hash`
      * **Teste do Banco de Registradores:** `make test_bank`
      * **Teste de Métricas da CPU:** `make test_metrics`

## 🛠️ Comandos Úteis do Makefile

O `CMakeLists.txt` foi configurado para criar atalhos úteis que você pode usar com o `make`:

| Comando         | Função                                                               |
| --------------- | -------------------------------------------------------------------- |
| `make` ou `make all` | Compila todos os alvos (simulador e testes).                      |
| `make simulador`| Compila apenas o executável principal do simulador.                |
| `make run`      | Executa o simulador principal (`./simulador`).                       |
| `make test-all` | Executa todos os programas de teste em sequência.                    |
| `make check`    | Fornece uma saída simplificada indicando se cada teste passou ou falhou. |
| `make ajuda`    | Exibe uma lista com todos os comandos disponíveis.                   |
| `make clean`    | Remove todos os arquivos gerados pela compilação.                    |