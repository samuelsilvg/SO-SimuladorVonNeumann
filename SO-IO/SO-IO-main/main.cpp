#include "IOManager.h"
#include "shared_structs.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <cstdlib>

const int QUANTUM = 5;

int main() {
    std::cout << "Iniciando simulacao do sistema operacional." << std::endl;
    IOManager io_manager;

    std::vector<PCB> processes;
    processes.push_back({1, State::Ready, 50, 0});
    processes.push_back({2, State::Ready, 40, 0});
    processes.push_back({3, State::Ready, 60, 0});

    int simulation_clock = 0;

    while (true) {
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

        PCB* process_to_run = nullptr;
        for (auto& p : processes) {
            if (p.state == State::Ready) {
                process_to_run = &p;
                break;
            }
        }

        if (process_to_run) {
            process_to_run->state = State::Executing;
            std::cout << "\n[Clock: " << simulation_clock << "] CPU: Executando Processo " << process_to_run->id << std::endl;

            for (int i = 0; i < QUANTUM; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                process_to_run->cycles_executed++;
                simulation_clock++;

                if (rand() % 8 == 0) {
                    std::cout << "CPU: Processo " << process_to_run->id << " solicitou I/O. BLOQUEADO e esperando por dispositivo." << std::endl;
                    process_to_run->state = State::Blocked;
                    
                    // O processo agora apenas se registra na lista de espera do IOManager
                    io_manager.registerProcessWaitingForIO(process_to_run);
                    
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
                std::cout << "CPU: Quantum do Processo " << process_to_run->id << " acabou. Estado: PRONTO." << std::endl;
            }
        } else {
            std::cout << "\n[Clock: " << simulation_clock << "] CPU Ociosa... Esperando processos ficarem prontos." << std::endl;
            simulation_clock++;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    return 0;
}