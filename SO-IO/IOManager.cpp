#include "IOManager.h"
#include <iostream>
#include <chrono>
#include <fstream>

// Construtor
IOManager::IOManager() : shutdown_flag(false) {
    // Abre arquivos de saída (modo append para acumular execuções)
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

void IOManager::addRequest(std::unique_ptr<IORequest> request) {
    std::lock_guard<std::mutex> lock(queueLock);
    requests.push_back(std::move(request));
}

void IOManager::managerLoop() {
    while (!shutdown_flag) {
        std::unique_ptr<IORequest> req = nullptr;

        {
            std::lock_guard<std::mutex> lock(queueLock);
            if (!requests.empty()) {
                req = std::move(requests.front());
                requests.erase(requests.begin());
            }
        }

        if (req) {
            auto start = std::chrono::steady_clock::now();

            std::this_thread::sleep_for(req->cost_cycles);

            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

            // ===== Saída para console =====
            std::cout << "I/O Manager: Processo " << req->process->id
                      << " executou operação '" << req->operation
                      << "' com msg: " << req->msg << std::endl;

            // ===== Escrita em result.dat =====
            resultFile << "Processo " << req->process->id
                       << " -> " << req->operation
                       << " : " << req->msg << std::endl;

            // ===== Escrita em output.dat (CSV) =====
            outputFile << req->process->id << ","
                       << req->operation << ","
                       << duration << "ms" << std::endl;

            // Libera o processo
            req->process->state = State::Ready;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}
