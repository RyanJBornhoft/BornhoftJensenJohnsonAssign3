#include "OpcodeTable.h"
#include <algorithm>
#include <cctype>

static std::string up(std::string s) {
    for (char &c : s) c = std::toupper((unsigned char)c);
    return s;
}

std::string OpcodeTable::normalize(const std::string& m) {
    std::string s = m;
    if (!s.empty() && s[0] == '+') s.erase(s.begin()); // strip format-4 marker
    return up(s);
}

OpcodeTable::OpcodeTable() {
    // format 3/4 (default 3; '+' selects 4)
    table["ADD"]   = {0x18,3}; table["ADDF"] = {0x58,3}; table["AND"]  = {0x40,3};
    table["COMP"]  = {0x28,3}; table["COMPF"]= {0x88,3};
    table["DIV"]   = {0x24,3}; table["DIVF"] = {0x64,3};
    table["J"]     = {0x3C,3}; table["JEQ"]  = {0x30,3}; table["JGT"]  = {0x34,3};
    table["JLT"]   = {0x38,3}; table["JSUB"] = {0x48,3};
    table["LDA"]   = {0x00,3}; table["LDB"]  = {0x68,3}; table["LDCH"] = {0x50,3};
    table["LDF"]   = {0x70,3}; table["LDL"]  = {0x08,3}; table["LDS"]  = {0x6C,3};
    table["LDT"]   = {0x74,3}; table["LDX"]  = {0x04,3}; table["LPS"]  = {0xD0,3};
    table["MUL"]   = {0x20,3}; table["MULF"] = {0x60,3}; table["OR"]   = {0x44,3};
    table["RD"]    = {0xD8,3}; table["RSUB"] = {0x4C,3}; table["SSK"]  = {0xEC,3};
    table["STA"]   = {0x0C,3}; table["STB"]  = {0x78,3}; table["STCH"] = {0x54,3};
    table["STF"]   = {0x80,3}; table["STI"]  = {0xD4,3}; table["STL"]  = {0x14,3};
    table["STS"]   = {0x7C,3}; table["STSW"] = {0xE8,3}; table["STT"]  = {0x84,3};
    table["STX"]   = {0x10,3}; table["SUB"]  = {0x1C,3}; table["SUBF"] = {0x5C,3};
    table["TD"]    = {0xE0,3}; table["TIX"]  = {0x2C,3}; table["WD"]   = {0xDC,3};

    // format 2
    table["ADDR"]  = {0x90,2}; table["CLEAR"]= {0xB4,2}; table["COMPR"]= {0xA0,2};
    table["DIVR"]  = {0x9C,2}; table["MULR"] = {0x98,2}; table["RMO"]  = {0xAC,2};
    table["SHIFTL"]= {0xA4,2}; table["SHIFTR"]= {0xA8,2}; table["SUBR"]= {0x94,2};
    table["SVC"]   = {0xB0,2}; table["TIXR"] = {0xB8,2};

    // format 1
    table["FIX"]   = {0xC4,1}; table["FLOAT"]= {0xC0,1}; table["HIO"]  = {0xF4,1};
    table["NORM"]  = {0xC8,1}; table["SIO"]  = {0xF0,1}; table["TIO"]  = {0xF8,1};
}

bool OpcodeTable::exists(const std::string& mnemonic) const {
    return table.find(normalize(mnemonic)) != table.end();
}
int OpcodeTable::getFormat(const std::string& mnemonic) const {
    if (!mnemonic.empty() && mnemonic[0] == '+') return 4;
    auto it = table.find(normalize(mnemonic));
    return it == table.end() ? -1 : it->second.format;
}
int OpcodeTable::getOpcode(const std::string& mnemonic) const {
    auto it = table.find(normalize(mnemonic));
    return it == table.end() ? -1 : it->second.opcode;
}