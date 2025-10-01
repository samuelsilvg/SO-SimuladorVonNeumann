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

    void addRequest(std::unique_ptr<IORequest> request);

private:
    void managerLoop();

    std::vector<std::unique_ptr<IORequest>> requests;
    std::mutex queueLock;
    bool shutdown_flag;
    std::thread managerThread;

    // Arquivos de saída
    std::ofstream resultFile;
    std::ofstream outputFile;
};

#endif // IOMANAGER_H
