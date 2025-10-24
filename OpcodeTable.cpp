#include "OpcodeTable.h"
#include <map>
#include <string>
#include <iostream>

struct OpcodeInfo {
    int opcode;
    int format;
};

class OpcodeTableImpl {
public:
    std::map<std::string, OpcodeInfo> opcodes;
    
    OpcodeTableImpl() {
        // Initialize with SIC/XE instruction set
        opcodes = {
            {"ADD",    {0x18, 3}},
            {"CLEAR",  {0xB4, 2}},
            {"COMP",   {0x28, 3}},
            {"J",      {0x3C, 3}},
            {"JEQ",    {0x30, 3}},
            {"JLT",    {0x38, 3}},
            {"JSUB",   {0x48, 3}},
            {"LDA",    {0x00, 3}},
            {"LDB",    {0x68, 3}},
            {"LDCH",   {0x50, 3}},
            {"LDT",    {0x74, 3}},
            {"RD",     {0xD8, 3}},
            {"RSUB",   {0x4C, 3}},
            {"STA",    {0x0C, 3}},
            {"STCH",   {0x54, 3}},
            {"STL",    {0x14, 3}},
            {"STX",    {0x10, 3}},
            {"TD",     {0xE0, 3}},
            {"TIXR",   {0xB8, 2}},
            {"WD",     {0xDC, 3}}
        };
    }
};

/********************************************************************
*** FUNCTION OpcodeTable (constructor)                            ***
*********************************************************************
*** DESCRIPTION : Initializes the opcode lookup table with        ***
***               standard SIC/XE mnemonics and formats.          ***
*** INPUT ARGS  : none                                            ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : (constructor)                                   ***
********************************************************************/

// Use PIMPL to ensure proper initialization
OpcodeTable::OpcodeTable() : pimpl(new OpcodeTableImpl()) {}

/********************************************************************
*** FUNCTION exists                                               ***
*********************************************************************
*** DESCRIPTION : Checks if a given opcode string is recognized   ***
***               in the table (case-insensitive or normalized).  ***
*** INPUT ARGS  : opcode - operation mnemonic                     ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : bool - true if opcode is valid; false otherwise ***
********************************************************************/

bool OpcodeTable::exists(const std::string& op) const {
    // Remove + prefix if present
    std::string baseOp = op;
    if (!op.empty() && op[0] == '+') {
        baseOp = op.substr(1);
    }
    
    return pimpl->opcodes.find(baseOp) != pimpl->opcodes.end();
}

/********************************************************************
*** FUNCTION getFormat                                            ***
*********************************************************************
*** DESCRIPTION : Returns the instruction format (1, 2, 3, or 4)  ***
***               for the given opcode.                           ***
*** INPUT ARGS  : opcode - operation mnemonic                     ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : int - format number; typically 3 for most ops   ***
********************************************************************/

int OpcodeTable::getFormat(const std::string& op) const {
    // Handle extended format
    if (!op.empty() && op[0] == '+') {
        return 4;
    }
    
    auto it = pimpl->opcodes.find(op);
    if (it != pimpl->opcodes.end()) {
        return it->second.format;
    }
    return 3;  // Default format
}

// Add destructor to properly clean up pimpl
OpcodeTable::~OpcodeTable() {
    delete pimpl;
}