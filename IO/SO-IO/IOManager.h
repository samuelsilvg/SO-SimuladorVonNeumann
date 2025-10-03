#ifndef IOMANAGER_H
#define IOMANAGER_H

#include "shared_structs.h"
#include <vector>
#include <mutex>
#include <thread>
#include <memory>
#include <fstream>

class IOManager {
public:
    IOManager();
    ~IOManager();

    // Método para um processo se registrar como "esperando por I/O"
    void registerProcessWaitingForIO(PCB* process);

private:
    void managerLoop();
    void addRequest(std::unique_ptr<IORequest> request);

    // Fila de requisições prontas para serem executadas
    std::vector<std::unique_ptr<IORequest>> requests;
    std::mutex queueLock;

    // Fila de processos que estão no estado BLOCKED esperando por um dispositivo
    std::vector<PCB*> waiting_processes;
    std::mutex waiting_processes_lock;

    // Variáveis booleanas representando o estado de cada dispositivo (0/1)
    bool printer_requesting;
    bool disk_requesting;
    bool network_requesting;
    std::mutex device_state_lock;

    bool shutdown_flag;
    std::thread managerThread;

    std::ofstream resultFile;
    std::ofstream outputFile;
};

#endif // IOMANAGER_H