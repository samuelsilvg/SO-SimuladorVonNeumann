#include "MainMemory.hpp"
#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>

void MainMemory::WriteMem(int addr, int value) {
    mem_[addr] = value;
}

int MainMemory::ReadMem(int addr) const {
    auto it = mem_.find(addr);
    if (it == mem_.end()) return 0;
    return it->second;
}

void MainMemory::Clear() {
    mem_.clear();
}

void MainMemory::DumpMem() const {
    std::vector<int> addrs; addrs.reserve(mem_.size());
    for (const auto& kv : mem_) addrs.push_back(kv.first);
    std::sort(addrs.begin(), addrs.end());

    std::cout << "=== Dump da Memória (words 32b) ===\n";
    for (int a : addrs) {
        int v = mem_.at(a);
        std::cout << "Addr " << std::dec << a
                  << " : 0x" << std::hex << std::setw(8) << std::setfill('0') << v
                  << std::dec << "\n";
    }
}
