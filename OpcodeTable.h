#pragma once
#include <string>
#include <map>

class OpcodeTable {
public:
    OpcodeTable();

    // Query (mnemonic may be "+JSUB" for format 4)
    bool exists(const std::string& mnemonic) const;
    int  getFormat(const std::string& mnemonic) const; // 1,2,3 (use '+' prefix => 4)
    int  getOpcode(const std::string& mnemonic) const; // 8-bit opcode (0x00..0xFF)

private:
    struct Entry { int opcode; int format; }; // format: 1,2,3 (3 means 3/4)
    std::map<std::string, Entry> table;       // keys uppercased without '+'
    static std::string normalize(const std::string& m);
};