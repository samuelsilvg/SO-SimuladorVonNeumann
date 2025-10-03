#include "parser_json.hpp"
#include "MainMemory.hpp"
#include <unordered_map>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <iomanip>

using namespace std;
using nlohmann::json;

// ======= Tabelas =======
const unordered_map<string, int> instructionMap = {
    // R-type (opcode 0)
    {"add",0}, {"sub",0}, {"and",0}, {"or",0}, {"mult",0}, {"div",0}, {"sll",0}, {"srl",0}, {"jr",0},
    // I-type
    {"addi",0b001000}, {"andi",0b001100}, {"ori",0b001101}, {"slti",0b001010},
    {"lw",0b100011}, {"sw",0b101011}, {"beq",0b000100}, {"bne",0b000101},
    // extras do seu schema
    {"bgt",0b000111}, {"blt",0b001001}, {"li",0b001111}, {"print",0b111110}, {"end",0b111111},
    // J-type
    {"j",0b000010}, {"jal",0b000011}
};

const unordered_map<string, int> functMap = {
    {"add",0b100000}, {"sub",0b100010}, {"and",0b100100}, {"or",0b100101},
    {"mult",0b011000}, {"div",0b011010}, {"sll",0b000000}, {"srl",0b000010}, {"jr",0b001000}
};

const unordered_map<string, int> registerMap = {
    {"$zero",0},{"$at",1},{"$v0",2},{"$v1",3},
    {"$a0",4},{"$a1",5},{"$a2",6},{"$a3",7},
    {"$t0",8},{"$t1",9},{"$t2",10},{"$t3",11},{"$t4",12},{"$t5",13},{"$t6",14},{"$t7",15},
    {"$s0",16},{"$s1",17},{"$s2",18},{"$s3",19},{"$s4",20},{"$s5",21},{"$s6",22},{"$s7",23},
    {"$t8",24},{"$t9",25},{"$k0",26},{"$k1",27},{"$gp",28},{"$sp",29},{"$fp",30},{"$ra",31}
};

// Mapas globais (dados e labels)
static unordered_map<string, int> dataMap;   // label -> endereço (byte)
static unordered_map<string, int> labelMap;  // label -> índice de instrução

// ======= Utils =======
string toLower(string s){
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c){return std::tolower(c);});
    return s;
}

int16_t parseImmediate(const json &j){
    if (j.is_string()){
        string s = toLower(j.get<string>());
        if (s.rfind("0x",0)==0) return static_cast<int16_t>(std::stoul(s,nullptr,16));
        return static_cast<int16_t>(std::stoi(s));
    }
    return static_cast<int16_t>(j.get<int>());
}

pair<int16_t,int> parseOffsetBase(const string &addrExpr){
    auto l = addrExpr.find('(');
    auto r = addrExpr.find(')');
    if (l==string::npos || r==string::npos || r<=l+1)
        throw runtime_error("Endereço inválido (esperado offset(base)): " + addrExpr);
    int16_t off = static_cast<int16_t>(std::stoi(addrExpr.substr(0,l)));
    string base = addrExpr.substr(l+1, r-l-1);
    auto it = registerMap.find(toLower(base));
    if (it==registerMap.end()) throw runtime_error("Registrador base inválido: " + base);
    return {off, it->second};
}

// ======= Helpers =======
int getRegisterCode(const string &reg){
    auto it = registerMap.find(toLower(reg));
    if (it!=registerMap.end()) return it->second;
    throw runtime_error("Registrador desconhecido: " + reg);
}

int getOpcode(const string &instr){
    auto it = instructionMap.find(toLower(instr));
    if (it!=instructionMap.end()) return it->second;
    throw runtime_error("Instrução desconhecida: " + instr);
}

int getFunct(const string &instr){
    auto it = functMap.find(toLower(instr));
    return (it!=functMap.end())? it->second : 0;
}

uint32_t buildBinaryInstruction(int opcode, int rs, int rt, int rd, int shamt, int funct,
                                int immediate, int address)
{
    if (opcode == 0){ // R
        uint32_t w=0;
        w |= (opcode & 0x3F) << 26;
        w |= (rs     & 0x1F) << 21;
        w |= (rt     & 0x1F) << 16;
        w |= (rd     & 0x1F) << 11;
        w |= (shamt  & 0x1F) <<  6;
        w |= (funct  & 0x3F);
        return w;
    } else if (opcode == 0b000010 || opcode == 0b000011){ // J/JAL
        uint32_t w=0;
        w |= (opcode & 0x3F) << 26;
        w |= (address & 0x03FFFFFF);
        return w;
    } else { // I
        uint32_t w=0;
        w |= (opcode & 0x3F) << 26;
        w |= (rs     & 0x1F) << 21;
        w |= (rt     & 0x1F) << 16;
        w |= (static_cast<uint16_t>(immediate));
        return w;
    }
}

// ======= Encoders =======
uint32_t encodeRType(const json &j){
    const string mnem = j.at("instruction").get<string>();
    int opcode = getOpcode(mnem);
    int funct  = getFunct(mnem);
    int rs=0, rt=0, rd=0, sh=0;

    if (mnem=="sll" || mnem=="srl"){
        rd = getRegisterCode(j.at("rd").get<string>());
        rt = getRegisterCode(j.at("rt").get<string>());
        sh = parseImmediate(j.at("shamt"));
    } else if (mnem=="jr"){
        rs = getRegisterCode(j.at("rs").get<string>());
    } else {
        rd = getRegisterCode(j.at("rd").get<string>());
        rs = getRegisterCode(j.at("rs").get<string>());
        rt = getRegisterCode(j.at("rt").get<string>());
    }
    return buildBinaryInstruction(opcode, rs, rt, rd, sh, funct, 0, 0);
}

uint32_t encodeIType(const json &j, int pcIdx){
    string mnem = j.at("instruction").get<string>();
    int opcode  = getOpcode(mnem);
    int rs=0, rt=0; int16_t imm=0;

    if (mnem=="li"){ // pseudo
        opcode = getOpcode("addi");
        rt = getRegisterCode(j.at("rt").get<string>());
        rs = getRegisterCode("$zero");
        imm = parseImmediate(j.at("immediate"));
        return buildBinaryInstruction(opcode, rs, rt, 0, 0, 0, imm, 0);
    }

    if (mnem=="lw" || mnem=="sw"){
        rt = getRegisterCode(j.at("rt").get<string>());
        if (j.contains("addr")){
            auto pr = parseOffsetBase(j.at("addr").get<string>());
            imm = pr.first; rs = pr.second;
        } else if (j.contains("baseReg")){
            rs = getRegisterCode(j.at("baseReg").get<string>());
            imm = j.contains("offset") ? parseImmediate(j.at("offset")) : 0;
        } else if (j.contains("base")){ // label de dados absoluto
            rs = getRegisterCode("$zero");
            const string lbl = j.at("base").get<string>();
            if (!dataMap.count(lbl)) throw runtime_error("Label de dados desconhecida: " + lbl);
            imm = static_cast<int16_t>(dataMap[lbl] & 0xFFFF);
        } else {
            throw runtime_error("lw/sw precisam de 'addr' ou 'baseReg' ou 'base'");
        }
        return buildBinaryInstruction(opcode, rs, rt, 0, 0, 0, imm, 0);
    }

    if (mnem=="beq" || mnem=="bne" || mnem=="bgt" || mnem=="blt"){
        rs = getRegisterCode(j.at("rs").get<string>());
        rt = getRegisterCode(j.at("rt").get<string>());
        if (j.contains("label")){
            const string lbl = j.at("label").get<string>();
            if (!labelMap.count(lbl)) throw runtime_error("Label desconhecida: " + lbl);
            imm = static_cast<int16_t>(labelMap[lbl] - (pcIdx + 1));
        } else if (j.contains("offset")){
            imm = parseImmediate(j.at("offset"));
        } else {
            throw runtime_error(mnem + " requer 'label' ou 'offset'");
        }
        return buildBinaryInstruction(opcode, rs, rt, 0, 0, 0, imm, 0);
    }

    // addi/andi/ori/slti
    rt  = getRegisterCode(j.at("rt").get<string>());
    rs  = getRegisterCode(j.at("rs").get<string>());
    imm = parseImmediate(j.at("immediate"));
    return buildBinaryInstruction(opcode, rs, rt, 0, 0, 0, imm, 0);
}

uint32_t encodeJType(const json &j){
    const string mnem = j.at("instruction").get<string>();
    int opcode = getOpcode(mnem);

    if (j.contains("label")){
        const string lbl = j.at("label").get<string>();
        if (!labelMap.count(lbl)) throw runtime_error("Label desconhecida (J): " + lbl);
        int addr = labelMap[lbl] & 0x03FFFFFF; // simplificado: índice
        return buildBinaryInstruction(opcode, 0,0,0,0,0, 0, addr);
    }
    if (j.contains("address")){
        uint32_t addr=0;
        if (j["address"].is_string()){
            string s = toLower(j["address"].get<string>());
            addr = (s.rfind("0x",0)==0)? std::stoul(s,nullptr,16) : static_cast<uint32_t>(std::stoul(s));
        } else {
            addr = j["address"].get<uint32_t>();
        }
        return buildBinaryInstruction(opcode, 0,0,0,0,0, 0, (addr & 0x03FFFFFF));
    }
    throw runtime_error("J-type requer 'label' ou 'address'");
}

// ======= Parser de instrução =======
uint32_t parseInstruction(const json &instrJson, int currentInstrIndex){
    const string mnem = instrJson.at("instruction").get<string>();
    if (mnem=="end" || mnem=="print")  // custom simples
        return static_cast<uint32_t>(getOpcode(mnem)) << 26;

    if (functMap.count(mnem))              return encodeRType(instrJson);
    if (mnem=="j" || mnem=="jal")          return encodeJType(instrJson);
    return encodeIType(instrJson, currentInstrIndex);
}

// ======= Seções =======
int parseData(const json &dataJson, MainMemory &ram, int startAddr){
    int addr = startAddr;

    // Formato A: objeto {"x":10,"y":[1,2]}
    if (dataJson.is_object()){
        for (auto it = dataJson.begin(); it != dataJson.end(); ++it){
            const string key = it.key();
            const json& val  = it.value();
            dataMap[key] = addr;
            if (val.is_array()){
                for (auto &e : val){
                    int w = e.is_string()? static_cast<int>(std::stoul(e.get<string>(),nullptr,0))
                                          : e.get<int>();
                    std::cout << "[DEBUG DATA] Escrevendo valor " << w << " no endereço " << addr << std::endl;
                    ram.WriteMem(addr, w); addr += 1;
                }
            } else {
                int w = val.is_string()? static_cast<int>(std::stoul(val.get<string>(),nullptr,0))
                                        : val.get<int>();
                ram.WriteMem(addr, w); addr += 1;
            }
        }
        return addr;
    }

    // Formato B: array de objetos [{label,type,value}]
    if (dataJson.is_array()){
        vector<uint8_t> bytes;
        auto flushBytes = [&](){
            for (size_t i=0;i<bytes.size(); i+=4){
                uint32_t w=0;
                for (size_t j=0;j<4 && i+j<bytes.size(); ++j) w = (w<<8) | bytes[i+j];
                ram.WriteMem(addr, w); addr += 1;
            }
            bytes.clear();
        };
        for (const auto &item : dataJson){
            string type = toLower(item.value("type","word"));
            string label = item.value("label", string());
            if (!label.empty()) dataMap[label] = addr;

            if (type=="word"){
                flushBytes();
                if (item["value"].is_array()){
                    for (auto &v : item["value"]){
                        int w = v.is_string()? static_cast<int>(std::stoul(v.get<string>(),nullptr,0))
                                             : v.get<int>();
                        ram.WriteMem(addr, w); addr += 1;
                    }
                } else {
                    int w = item["value"].is_string()? static_cast<int>(std::stoul(item["value"].get<string>(),nullptr,0))
                                                      : item["value"].get<int>();
                    std::cout << "[DEBUG DATA] Escrevendo valor " << w << " no endereço " << addr << std::endl;
                    ram.WriteMem(addr, w); addr += 1;
                }
            } else if (type=="byte"){
                if (item["value"].is_array()){
                    for (auto &v : item["value"]){
                        uint8_t b = v.is_string()? static_cast<uint8_t>(std::stoul(v.get<string>(),nullptr,0))
                                                  : static_cast<uint8_t>(v.get<int>());
                        bytes.push_back(b);
                    }
                } else {
                    uint8_t b = item["value"].is_string()? static_cast<uint8_t>(std::stoul(item["value"].get<string>(),nullptr,0))
                                                          : static_cast<uint8_t>(item["value"].get<int>());
                    bytes.push_back(b);
                }
            }
        }
        flushBytes();
    }
    return addr;
}

int parseProgram(const json &programJson, MainMemory &ram, int startAddr) {
    if (!programJson.is_array()) {
        return startAddr;
    }

    // 1) Mapear todas as labels para seus endereços de instrução.
    //    Este loop percorre o programa para encontrar todas as labels antes de
    //    começar a traduzir as instruções.
    int instruction_address_counter = 0;
    for (const auto &node : programJson) {
        // Se o objeto JSON atual tiver uma chave "label"...
        if (node.contains("label")) {
            // ...salvamos o nome da label e o endereço da instrução atual no mapa.
            labelMap[node["label"].get<string>()] = instruction_address_counter;
        }

        // O contador de endereço só avança se houver de fato uma instrução na linha.
        // Isso garante que os endereços das labels fiquem corretos.
        if (node.contains("instruction")) {
            instruction_address_counter++;
        }
    }


    // 2) Codificar e escrever as instruções na memória.
    //    Este loop permanece como você fez, pois a lógica está correta.
    int current_mem_addr = startAddr;
    int current_instruction_addr = 0;
    for (const auto &node : programJson) {
        // Pula linhas que são apenas labels (se houver alguma)
        if (!node.contains("instruction")) {
            continue;
        }
        
        // Traduz a instrução para binário
        uint32_t binary_instruction = parseInstruction(node, current_instruction_addr);
        
        // Escreve a instrução na memória
        ram.WriteMem(current_mem_addr, binary_instruction);
        
        // Avança para o próximo endereço de memória (palavra de 4 bytes)
        current_mem_addr += 1;
        current_instruction_addr++;
    }

    return current_mem_addr;
}

// ======= Loader =======
static json readJsonFile(const string &filename){
    ifstream f(filename);
    if (!f) throw runtime_error("Não foi possível abrir: " + filename);
    json j; f >> j; return j;
}

int loadJsonProgram(const string &filename, MainMemory &ram, int startAddr){
    dataMap.clear();
    labelMap.clear();

    json j = readJsonFile(filename);
    int addr = startAddr;
    if (j.contains("data"))    addr = parseData(j["data"],    ram, addr);
    if (j.contains("program")) addr = parseProgram(j["program"], ram, addr);
    return addr;
}
