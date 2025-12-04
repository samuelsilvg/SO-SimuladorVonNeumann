#include <iostream>
#include <vector>
#include <deque>
#include <memory>
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>


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
    // Marca o início da execução
    auto start_time = std::chrono::high_resolution_clock::now();

    // 1. Inicialização dos Módulos Principais
    std::cout << "Inicializando o simulador...\n";
    MemoryManager memManager(1024, 8192);
    IOManager ioManager;

    // 2. Carregamento dos Processos
    std::vector<std::unique_ptr<PCB>> process_list;
    std::deque<PCB*> ready_queue;
    std::vector<PCB*> blocked_list;

    // Carrega todos os processos da pasta processes/
    std::vector<std::string> process_files = {
        "process_balanced", "process_cpu_bound", "process_io_bound",
        "process_long", "process_loop_heavy", "process_medium",
        "process_memory_intensive", "process_quick", "process_short"
    };

    for (const auto& process_name : process_files) {
        auto pcb = std::make_unique<PCB>();
        std::string process_path = "../processes/" + process_name + ".json";
        std::string tasks_path = "../tasks/tasks_" + process_name.substr(8) + ".json"; // remove "process_"
        
        if (load_pcb_from_json(process_path, *pcb)) {
            std::cout << "Carregando programa '" << tasks_path << "' para o processo " << pcb->pid << "...\n";
            loadJsonProgram(tasks_path, memManager, *pcb, 0);
            process_list.push_back(std::move(pcb));
        } else {
            std::cerr << "Aviso: Não foi possível carregar '" << process_path << "'.\n";
        }
    }

    // Adiciona os processos na fila de prontos
    for (const auto& process : process_list) {
        ready_queue.push_back(process.get());
    }

    int total_processes = process_list.size();
    int finished_processes = 0;

    // Métricas de desempenho
    uint64_t total_wait_time = 0;
    uint64_t total_execution_time = 0;
    uint64_t total_cpu_cycles = 0;
    uint64_t total_idle_cycles = 0;
    auto scheduler_start = std::chrono::high_resolution_clock::now();

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
            // CPU ociosa - conta ciclos de idle
            total_idle_cycles += 50; // Aproximação baseada no sleep
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        PCB* current_process = ready_queue.front();
        ready_queue.pop_front();

        std::cout << "\n[Scheduler] Executando processo " << current_process->pid << " (Quantum: " << current_process->quantum << ").\n";
        
        // Marca início da execução deste quantum
        auto exec_start = std::chrono::high_resolution_clock::now();
        current_process->state = State::Running;

        std::vector<std::unique_ptr<IORequest>> io_requests;
        bool print_lock = true;

        // Executa o núcleo da CPU
        Core(memManager, *current_process, &io_requests, print_lock);

        // Calcula tempo de execução deste quantum
        auto exec_end = std::chrono::high_resolution_clock::now();
        auto exec_duration = std::chrono::duration_cast<std::chrono::microseconds>(exec_end - exec_start).count();
        
        // Atualiza métricas
        total_cpu_cycles += current_process->pipeline_cycles.load();

        // Avalia o estado do processo após a execução
        switch (current_process->state) {
            case State::Blocked:
                std::cout << "[Scheduler] Processo " << current_process->pid << " bloqueado por I/O. Entregando ao IOManager.\n";
                ioManager.registerProcessWaitingForIO(current_process);
                blocked_list.push_back(current_process);
                break;

            case State::Finished:
                std::cout << "[Scheduler] Processo " << current_process->pid << " finalizado.\n";
                
                // Coleta métricas do processo finalizado
                total_execution_time += current_process->pipeline_cycles.load();
                
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

    // Calcula tempo de scheduler
    auto scheduler_end = std::chrono::high_resolution_clock::now();
    auto scheduler_duration = std::chrono::duration_cast<std::chrono::milliseconds>(scheduler_end - scheduler_start);

    // Calcula e exibe o tempo total de execução
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // ========== MÉTRICAS DE DESEMPENHO ==========
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║           MÉTRICAS DE DESEMPENHO DO SIMULADOR            ║\n";
    std::cout << "╠════════════════════════════════════════════════════════════╣\n";
    
    // Tempo total de execução
    std::cout << "║ Tempo Total de Execução:                                 ║\n";
    std::cout << "║   • " << duration.count() << " ms (" << duration.count() / 1000.0 << " segundos)";
    std::cout << std::string(31 - std::to_string(duration.count()).length(), ' ') << "║\n";
    std::cout << "╠════════════════════════════════════════════════════════════╣\n";
    
    // Cálculo das métricas
    double avg_execution_time = total_processes > 0 ? 
        static_cast<double>(total_execution_time) / total_processes : 0.0;
    
    // Tempo médio de espera (estimado baseado em ciclos e quantum)
    uint64_t total_turnaround = 0;
    for (const auto& process : process_list) {
        total_turnaround += process->pipeline_cycles.load() + process->memory_cycles.load();
    }
    double avg_turnaround_time = total_processes > 0 ? 
        static_cast<double>(total_turnaround) / total_processes : 0.0;
    double avg_wait_time = avg_turnaround_time - avg_execution_time;
    
    // Utilização da CPU
    uint64_t total_cycles = total_cpu_cycles + total_idle_cycles;
    double cpu_utilization = total_cycles > 0 ? 
        (static_cast<double>(total_cpu_cycles) / total_cycles) * 100.0 : 0.0;
    
    // Throughput (processos por segundo)
    double throughput = scheduler_duration.count() > 0 ? 
        (static_cast<double>(total_processes) / (scheduler_duration.count() / 1000.0)) : 0.0;
    
    // Eficiência (razão entre tempo útil e tempo total)
    double efficiency = total_turnaround > 0 ? 
        (static_cast<double>(total_execution_time) / total_turnaround) * 100.0 : 0.0;
    
    std::cout << "║ 1. Tempo Médio de Espera:                               ║\n";
    std::cout << "║   • " << std::fixed << std::setprecision(2) << avg_wait_time << " ciclos";
    std::cout << std::string(44 - std::to_string(static_cast<int>(avg_wait_time)).length(), ' ') << "║\n";
    std::cout << "╠════════════════════════════════════════════════════════════╣\n";
    
    std::cout << "║ 2. Tempo Médio de Execução:                             ║\n";
    std::cout << "║   • " << std::fixed << std::setprecision(2) << avg_execution_time << " ciclos";
    std::cout << std::string(44 - std::to_string(static_cast<int>(avg_execution_time)).length(), ' ') << "║\n";
    std::cout << "╠════════════════════════════════════════════════════════════╣\n";
    
    std::cout << "║ 3. Tempo Médio de Turnaround:                           ║\n";
    std::cout << "║   • " << std::fixed << std::setprecision(2) << avg_turnaround_time << " ciclos";
    std::cout << std::string(44 - std::to_string(static_cast<int>(avg_turnaround_time)).length(), ' ') << "║\n";
    std::cout << "╠════════════════════════════════════════════════════════════╣\n";
    
    std::cout << "║ 4. Utilização da CPU:                                   ║\n";
    std::cout << "║   • " << std::fixed << std::setprecision(2) << cpu_utilization << " %";
    std::cout << std::string(48 - std::to_string(static_cast<int>(cpu_utilization)).length(), ' ') << "║\n";
    std::cout << "║   • Ciclos Úteis: " << total_cpu_cycles;
    std::cout << std::string(39 - std::to_string(total_cpu_cycles).length(), ' ') << "║\n";
    std::cout << "║   • Ciclos Ociosos: " << total_idle_cycles;
    std::cout << std::string(37 - std::to_string(total_idle_cycles).length(), ' ') << "║\n";
    std::cout << "╠════════════════════════════════════════════════════════════╣\n";
    
    std::cout << "║ 5. Throughput (Taxa de Vazão):                          ║\n";
    std::cout << "║   • " << std::fixed << std::setprecision(2) << throughput << " processos/segundo";
    std::cout << std::string(34 - std::to_string(static_cast<int>(throughput)).length(), ' ') << "║\n";
    std::cout << "╠════════════════════════════════════════════════════════════╣\n";
    
    std::cout << "║ 6. Eficiência Global:                                   ║\n";
    std::cout << "║   • " << std::fixed << std::setprecision(2) << efficiency << " %";
    std::cout << std::string(48 - std::to_string(static_cast<int>(efficiency)).length(), ' ') << "║\n";
    std::cout << "╠════════════════════════════════════════════════════════════╣\n";
    
    std::cout << "║ Processos Executados: " << total_processes;
    std::cout << std::string(35 - std::to_string(total_processes).length(), ' ') << "║\n";
    
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    // Salvar métricas em arquivo
    std::ofstream metrics_file("../output/metrics_summary.dat");
    if (metrics_file.is_open()) {
        metrics_file << "=== RESUMO DE MÉTRICAS DE DESEMPENHO ===\n\n";
        metrics_file << "Tempo Total de Execução: " << duration.count() << " ms (" 
                     << duration.count() / 1000.0 << " s)\n";
        metrics_file << "Processos Executados: " << total_processes << "\n\n";
        metrics_file << "1. Tempo Médio de Espera: " << avg_wait_time << " ciclos\n";
        metrics_file << "2. Tempo Médio de Execução: " << avg_execution_time << " ciclos\n";
        metrics_file << "3. Tempo Médio de Turnaround: " << avg_turnaround_time << " ciclos\n";
        metrics_file << "4. Utilização da CPU: " << cpu_utilization << " %\n";
        metrics_file << "   - Ciclos Úteis: " << total_cpu_cycles << "\n";
        metrics_file << "   - Ciclos Ociosos: " << total_idle_cycles << "\n";
        metrics_file << "5. Throughput: " << throughput << " processos/segundo\n";
        metrics_file << "6. Eficiência Global: " << efficiency << " %\n";
        metrics_file.close();
        std::cout << "\n✓ Métricas salvas em '../output/metrics_summary.dat'\n";
    }

    

    return 0;
}