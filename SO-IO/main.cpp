#include "IOManager.h"
#include "shared_structs.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <cstdlib>

// O "Quantum" ou fatia de tempo que cada processo pode executar na CPU
const int QUANTUM = 5;

int main() {
    std::cout << "Iniciando simulacao do sistema operacional." << std::endl;

    IOManager io_manager;

    // Criação dos processos
    std::vector<PCB> processes;
    processes.push_back({1, State::Ready, 50, 0});
    processes.push_back({2, State::Ready, 40, 0});
    processes.push_back({3, State::Ready, 60, 0});

    int simulation_clock = 0;

    while (true) {
        // 1. Verifica se todos os processos terminaram
        bool all_finished = true;
        for (const auto& p : processes) {
            if (p.state != State::Finished) {
                all_finished = false;
                break;
            }
        }
        if (all_finished) {
            std::cout << "\nTodos os processos terminaram. Desligando." << std::endl;
            break;
        }

        // 2. Escalonador simples (Round-Robin)
        PCB* process_to_run = nullptr;
        for (auto& p : processes) {
            if (p.state == State::Ready) {
                process_to_run = &p;
                break;
            }
        }

        // 3. Simulação da CPU
        if (process_to_run) {
            process_to_run->state = State::Executing;
            std::cout << "\n[Clock: " << simulation_clock 
                      << "] CPU: Executando Processo " << process_to_run->id << std::endl;

            for (int i = 0; i < QUANTUM; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                process_to_run->cycles_executed++;
                simulation_clock++;

                // Simula uma chance de instrução especial de I/O (ex: print)
                if (rand() % 8 == 0) {
                    std::cout << "CPU: Processo " << process_to_run->id 
                              << " solicitou I/O. BLOQUEADO." << std::endl;
                    process_to_run->state = State::Blocked;

                    auto request = std::make_unique<IORequest>();
                    request->process = process_to_run;
                    request->operation = "write_result";
                    request->msg = "Saida simulada do processo.";
                    request->cost_cycles = std::chrono::milliseconds(200);

                    io_manager.addRequest(std::move(request));
                    break;
                }

                if (process_to_run->cycles_executed >= process_to_run->total_cycles) {
                    process_to_run->state = State::Finished;
                    std::cout << "CPU: Processo " << process_to_run->id << " TERMINOU." << std::endl;
                    break;
                }
            }

            if (process_to_run->state == State::Executing) {
                process_to_run->state = State::Ready;
                std::cout << "CPU: Quantum do Processo " << process_to_run->id 
                          << " acabou. Estado: PRONTO." << std::endl;
            }
        } else {
            std::cout << "\n[Clock: " << simulation_clock 
                      << "] CPU Ociosa... Esperando processos ficarem prontos." << std::endl;
            simulation_clock++;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    return 0;
}
