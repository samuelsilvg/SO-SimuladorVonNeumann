#include <iostream>
#include <vector>
#include <deque>
#include <memory>
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>


#include "cpu/PCB.hpp"
#include "cpu/pcb_loader.hpp"
#include "cpu/CONTROL_UNIT.hpp"
#include "memory/MemoryManager.hpp"
#include "parser_json/parser_json.hpp"
#include "IO/IOManager.hpp"

// Função para imprimir as métricas de um processo
void print_metrics(const PCB& pcb) {
    std::cout << "\n--- METRICAS FINAIS DO PROCESSO " << pcb.pid << " ---\n";
    std::cout << "Nome do Processo:       " << pcb.name << "\n";
    std::cout << "Estado Final:           " << (pcb.state == State::Finished ? "Finished" : "Incomplete") << "\n";
    std::cout << "Ciclos de Pipeline:     " << pcb.pipeline_cycles.load() << "\n";
    std::cout << "Total de Acessos a Mem: " << pcb.mem_accesses_total.load() << "\n";
    std::cout << "  - Leituras:             " << pcb.mem_reads.load() << "\n";
    std::cout << "  - Escritas:             " << pcb.mem_writes.load() << "\n";
    std::cout << "Acessos a Cache L1:     " << pcb.cache_mem_accesses.load() << "\n";
    std::cout << "Acessos a Mem Principal:" << pcb.primary_mem_accesses.load() << "\n";
    std::cout << "Acessos a Mem Secundaria:" << pcb.secondary_mem_accesses.load() << "\n";
    std::cout << "Ciclos Totais de Memoria: " << pcb.memory_cycles.load() << "\n";
    std::cout << "------------------------------------------\n";
    // cria pasta "output" se não existir
    std::filesystem::create_directory("output");

    // abre arquivos
    std::ofstream resultados("output/resultados.dat");
    std::ofstream output("output/output.dat");

    if (resultados.is_open()) {
        resultados << "=== Resultados de Execução ===\n";
        resultados << "PID: " << pcb.pid << "\n";
        resultados << "Nome: " << pcb.name << "\n";
        resultados << "Quantum: " << pcb.quantum << "\n";
        resultados << "Prioridade: " << pcb.priority << "\n";
        resultados << "Ciclos de Pipeline: " << pcb.pipeline_cycles << "\n";
        resultados << "Ciclos de Memória: " << pcb.memory_cycles << "\n";
        resultados << "Cache Hits: " << pcb.cache_hits << "\n";
        resultados << "Cache Misses: " << pcb.cache_misses << "\n";
        resultados << "Ciclos de IO: " << pcb.io_cycles << "\n";
    }


    if (output.is_open()) {
        output << "=== Saída Lógica do Programa ===\n";

        // Dump de registradores
        output << "Registradores principais:\n";
        output << pcb.regBank.get_registers_as_string() << "\n";

        // Inserir operações registradas
        output << "\n=== Operações Executadas ===\n";

        // Lê o arquivo temporário com operações
        std::string temp_filename = "output/temp_1.log";
        if (std::filesystem::exists(temp_filename)) {
            std::ifstream temp_file(temp_filename);
            if (temp_file.is_open()) {
                std::string line;
                while (std::getline(temp_file, line)) {
                    output << line << "\n";
                }
                temp_file.close();
            }

            // Remove arquivo temporário após consolidar
            std::filesystem::remove(temp_filename);
        } else {
            output << "(Nenhuma operação registrada)\n";
        }

        output << "\n=== Fim das Operações Registradas ===\n";
    }



    resultados.close();
    output.close();
}


int main() {
    // 1. Inicialização dos Módulos Principais
    std::cout << "Inicializando o simulador...\n";
    MemoryManager memManager(1024, 8192);
    IOManager ioManager;

    // 2. Carregamento dos Processos
    std::vector<std::unique_ptr<PCB>> process_list;
    std::deque<PCB*> ready_queue;
    std::vector<PCB*> blocked_list;

    // Carrega um processo a partir de um arquivo JSON
    auto p1 = std::make_unique<PCB>();
    // CORREÇÃO: Caminho simplificado
    if (load_pcb_from_json("process1.json", *p1)) {
        // CORREÇÃO: Caminho simplificado
        std::cout << "Carregando programa 'tasks.json' para o processo " << p1->pid << "...\n";
        loadJsonProgram("tasks.json", memManager, *p1, 0);
        process_list.push_back(std::move(p1));
    } else {
        std::cerr << "Erro ao carregar 'process1.json'. Certifique-se de que o arquivo está na pasta raiz do projeto.\n";
        return 1;
    }

    // Adiciona os processos na fila de prontos
    for (const auto& process : process_list) {
        ready_queue.push_back(process.get());
    }

    int total_processes = process_list.size();
    int finished_processes = 0;

    // 3. Loop Principal do Escalonador
    std::cout << "\nIniciando escalonador Round-Robin...\n";
    while (finished_processes < total_processes) {
        // Verifica se há processos que foram desbloqueados pelo IOManager
        for (auto it = blocked_list.begin(); it != blocked_list.end(); ) {
            if ((*it)->state == State::Ready) {
                std::cout << "[Scheduler] Processo " << (*it)->pid << " desbloqueado e movido para a fila de prontos.\n";
                ready_queue.push_back(*it);
                it = blocked_list.erase(it);
            } else {
                ++it;
            }
        }

        if (ready_queue.empty()) {
            if (blocked_list.empty()) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        PCB* current_process = ready_queue.front();
        ready_queue.pop_front();

        std::cout << "\n[Scheduler] Executando processo " << current_process->pid << " (Quantum: " << current_process->quantum << ").\n";
        current_process->state = State::Running;

        std::vector<std::unique_ptr<IORequest>> io_requests;
        bool print_lock = true;

        // Executa o núcleo da CPU
        Core(memManager, *current_process, &io_requests, print_lock);

        // Avalia o estado do processo após a execução
        switch (current_process->state) {
            case State::Blocked:
                std::cout << "[Scheduler] Processo " << current_process->pid << " bloqueado por I/O. Entregando ao IOManager.\n";
                ioManager.registerProcessWaitingForIO(current_process);
                blocked_list.push_back(current_process);
                break;

            case State::Finished:
                std::cout << "[Scheduler] Processo " << current_process->pid << " finalizado.\n";
                print_metrics(*current_process);
                finished_processes++;
                break;

            default:
                std::cout << "[Scheduler] Quantum do processo " << current_process->pid << " expirou. Voltando para a fila.\n";
                current_process->state = State::Ready;
                ready_queue.push_back(current_process);
                break;
        }
    }

    std::cout << "\nTodos os processos foram finalizados. Encerrando o simulador.\n";

    

    return 0;
}