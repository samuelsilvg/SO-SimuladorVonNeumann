/*
  pcb_loader.cpp
  Implementação do carregamento de PCB via JSON.
*/
#include "pcb_loader.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

bool load_pcb_from_json(const std::string &path, PCB &pcb) {
    std::ifstream f(path);
    if (!f.is_open()) return false;
    try {
        json j; f >> j;
        pcb.pid = j.value("pid", 0);
        pcb.name = j.value("name", std::string(""));
        pcb.quantum = j.value("quantum", 0);
        pcb.priority = j.value("priority", 0);
        if (j.contains("mem_weights")) {
            auto &mw = j["mem_weights"];
            pcb.memWeights.primary = mw.value("primary", 1ULL);
            pcb.memWeights.secondary = mw.value("secondary", 10ULL);
        }
        return true;
    } catch (...) {
        return false;
    }
}
