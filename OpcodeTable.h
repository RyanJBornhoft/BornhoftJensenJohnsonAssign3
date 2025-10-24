#ifndef OPCODE_TABLE_H
#define OPCODE_TABLE_H

#include <string>

class OpcodeTableImpl;  // Forward declaration for PIMPL

class OpcodeTable {
public:
    OpcodeTable();
    ~OpcodeTable();
    bool exists(const std::string& op) const;
    int getFormat(const std::string& op) const;

private:
    OpcodeTableImpl* pimpl;
};

#endif