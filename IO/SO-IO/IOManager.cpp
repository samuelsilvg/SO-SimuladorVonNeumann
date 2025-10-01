#include "IOManager.h"
#include <iostream>
#include <chrono>
#include <fstream>
#include <cstdlib>
#include <ctime>

// Construtor
IOManager::IOManager() :
    printer_requesting(false),
    disk_requesting(false),
    network_requesting(false),
    shutdown_flag(false)
{
    srand(time(nullptr));

    resultFile.open("result.dat", std::ios::app);
    outputFile.open("output.dat", std::ios::app);

    if (!resultFile || !outputFile) {
        std::cerr << "Erro: não foi possível abrir arquivos de saída." << std::endl;
    }

    managerThread = std::thread(&IOManager::managerLoop, this);
}

// Destrutor
IOManager::~IOManager() {
    shutdown_flag = true;
    if (managerThread.joinable()) {
        managerThread.join();
    }
    resultFile.close();
    outputFile.close();
}

// Adiciona um processo à lista de espera por I/O
void IOManager::registerProcessWaitingForIO(PCB* process) {
    std::lock_guard<std::mutex> lock(waiting_processes_lock);
    waiting_processes.push_back(process);
}

// Adiciona uma requisição criada à fila de processamento
void IOManager::addRequest(std::unique_ptr<IORequest> request) {
    std::lock_guard<std::mutex> lock(queueLock);
    requests.push_back(std::move(request));
}

void IOManager::managerLoop() {
    while (!shutdown_flag) {
        // ETAPA 1: Simula os dispositivos solicitando uma operação (mudando o estado para 1/true)
        {
            std::lock_guard<std::mutex> lock(device_state_lock);
            if (rand() % 100 == 0) {
                if (!printer_requesting) {
                    printer_requesting = true;
                    std::cout << "I/O Manager: [Impressora] está solicitando uma operação." << std::endl;
                }
            }
            if (rand() % 50 == 0) {
                if (!disk_requesting) {
                    disk_requesting = true;
                    std::cout << "I/O Manager: [Disco] está solicitando uma operação." << std::endl;
                }
            }
        }

        // ETAPA 2: Verifica se há dispositivos solicitando E processos esperando para atendê-los
        std::unique_ptr<IORequest> new_request = nullptr;
        {
            std::lock_guard<std::mutex> wplock(waiting_processes_lock);
            if (!waiting_processes.empty()) {
                std::lock_guard<std::mutex> dslock(device_state_lock);
                PCB* process_to_service = waiting_processes.front();

                if (printer_requesting) {
                    new_request = std::make_unique<IORequest>();
                    new_request->operation = "print_job";
                    new_request->msg = "Imprimindo documento...";
                    printer_requesting = false; // Atendido, estado volta para 0
                } else if (disk_requesting) {
                    new_request = std::make_unique<IORequest>();
                    new_request->operation = "read_from_disk";
                    new_request->msg = "Lendo dados do disco...";
                    disk_requesting = false; // Atendido, estado volta para 0
                }
                
                if (new_request) {
                    new_request->process = process_to_service;
                    waiting_processes.erase(waiting_processes.begin());
                    // Atribui o custo aleatório de 1 a 3
                    new_request->cost_cycles = std::chrono::milliseconds((rand() % 3 + 1) * 100);
                }
            }
        }
        
        if (new_request) {
            addRequest(std::move(new_request));
        }

        // ETAPA 3: Processa a próxima requisição na fila (se houver)
        std::unique_ptr<IORequest> req_to_process = nullptr;
        {
            std::lock_guard<std::mutex> lock(queueLock);
            if (!requests.empty()) {
                req_to_process = std::move(requests.front());
                requests.erase(requests.begin());
            }
        }

        if (req_to_process) {
            auto start = std::chrono::steady_clock::now();
            std::this_thread::sleep_for(req_to_process->cost_cycles);
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

            std::cout << "I/O Manager: Processo " << req_to_process->process->id << " executou '" << req_to_process->operation << "'" << std::endl;

            resultFile << "Processo " << req_to_process->process->id << " -> " << req_to_process->operation << " : " << req_to_process->msg << std::endl;
            outputFile << req_to_process->process->id << "," << req_to_process->operation << "," << duration << "ms" << std::endl;

            req_to_process->process->state = State::Ready;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(20)); // Dorme se não há nada a fazer
        }
    }
}