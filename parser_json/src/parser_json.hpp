#pragma once
#include <cstdint>
#include <string>
#include <utility>
#include <nlohmann/json.hpp>

using nlohmann::json;

class MainMemory;

// ===== API principal =====
int loadJsonProgram(const std::string &filename, MainMemory &ram, int startAddr);

// ===== Parsers de seção =====
int parseData(const json &dataJson, MainMemory &ram, int startAddr);
int parseProgram(const json &programJson, MainMemory &ram, int startAddr);

// ===== Parser de instrução =====
uint32_t parseInstruction(const json &instrJson, int currentInstrIndex);

// ===== Helpers / Encoders =====
int     getRegisterCode(const std::string &reg);
int     getOpcode(const std::string &instr);
int     getFunct(const std::string &instr);

uint32_t buildBinaryInstruction(int opcode, int rs, int rt, int rd, int shamt, int funct,
                                int immediate, int address);

uint32_t encodeRType(const nlohmann::json &instrJson);
uint32_t encodeIType(const nlohmann::json &instrJson, int currentInstrIndex);
uint32_t encodeJType(const nlohmann::json &instrJson);

// ===== Utils =====
std::pair<int16_t,int> parseOffsetBase(const std::string &addrExpr);
int16_t   parseImmediate(const json &j);
std::string toLower(std::string s);
