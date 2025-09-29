#ifndef PCB_LOADER_HPP
#define PCB_LOADER_HPP
/*
  pcb_loader.hpp
  Função utilitária para carregar um PCB a partir de um arquivo JSON.
  Requer a biblioteca nlohmann/json (header-only) instalada.
*/
#include <string>
#include "PCB.hpp"

bool load_pcb_from_json(const std::string &path, PCB &pcb);

#endif // PCB_LOADER_HPP
