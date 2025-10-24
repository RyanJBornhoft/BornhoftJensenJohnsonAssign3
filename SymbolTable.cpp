#include "SymbolTable.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <map>
#include <sstream>

class SymbolTableImpl {
public:
    struct Symbol {
        int value = 0;
        std::string valueStr;
        bool rflag = true;
        bool iflag = true;
        bool mflag = false;
    };
    std::map<std::string, Symbol> symbols;
};

/********************************************************************
*** FUNCTION key6                                                 ***
*********************************************************************
*** DESCRIPTION : Truncates a symbol name to 6 characters for     ***
***               standardized key lookups.                       ***
*** INPUT ARGS  : s - symbol name string                          ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : string - truncated (max 6 chars) copy           ***
********************************************************************/
static std::string key6(std::string s) {
    if (s.size() > 6) s = s.substr(0, 6);
    return s;
}

/********************************************************************
*** FUNCTION SymbolTable (constructor)                            ***
*********************************************************************
*** DESCRIPTION : Initializes an empty symbol table using the     ***
***               PIMPL idiom.                                    ***
*** INPUT ARGS  : none                                            ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : (constructor)                                   ***
********************************************************************/
SymbolTable::SymbolTable() : pimpl(new SymbolTableImpl()) {}

/********************************************************************
*** FUNCTION ~SymbolTable (destructor)                            ***
*********************************************************************
*** DESCRIPTION : Cleans up internal implementation object.       ***
*** INPUT ARGS  : none                                            ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : (destructor)                                    ***
********************************************************************/
SymbolTable::~SymbolTable() {
    delete pimpl;
}

/********************************************************************
*** FUNCTION insert                                               ***
*********************************************************************
*** DESCRIPTION : Inserts a symbol with numeric value and flags.  ***
***               Symbol names are truncated to 6 chars. Rejects  ***
***               duplicates.                                     ***
*** INPUT ARGS  : name   - symbol name                            ***
***               value  - numeric value (LOCCTR or EQU result)   ***
***               rflag  - relative flag (default true)           ***
***               iflag  - interpretation flag (default true)     ***
***               mflag  - modification flag (default false)      ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : bool - false if duplicate; true if inserted     ***
********************************************************************/
bool SymbolTable::insert(const std::string& name, int value,
                         bool rflag, bool iflag, bool mflag) {
    std::string key = key6(name);
    if (pimpl->symbols.find(key) != pimpl->symbols.end()) return false;
    SymbolTableImpl::Symbol sym;
    sym.value = value;
    sym.valueStr.clear();
    sym.rflag = rflag;
    sym.iflag = iflag;
    sym.mflag = mflag;
    pimpl->symbols[key] = sym;
    return true;
}

/********************************************************************
*** FUNCTION insertWithValueString                                ***
*********************************************************************
*** DESCRIPTION : Inserts a symbol with an explicit printable     ***
***               VALUE string (e.g., "200" or "F"). Useful for   ***
***               EQU or character constants. Truncates to 6      ***
***               chars; rejects duplicates.                      ***
*** INPUT ARGS  : name     - symbol name                          ***
***               valueStr - printable value string               ***
***               rflag    - relative flag (default true)         ***
***               iflag    - interpretation flag (default true)   ***
***               mflag    - modification flag (default false)    ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : bool - false if duplicate; true if inserted     ***
********************************************************************/
bool SymbolTable::insertWithValueString(const std::string& name, const std::string& valueStr,
                                        bool rflag, bool iflag, bool mflag) {
    std::string key = key6(name);
    if (pimpl->symbols.find(key) != pimpl->symbols.end()) return false;
    SymbolTableImpl::Symbol sym;
    sym.value = 0;
    sym.valueStr = valueStr;
    sym.rflag = rflag;
    sym.iflag = iflag;
    sym.mflag = mflag;
    pimpl->symbols[key] = sym;
    return true;
}

/********************************************************************
*** FUNCTION setMFlag                                             ***
*********************************************************************
*** DESCRIPTION : Sets the MFLAG for an existing symbol (e.g.,    ***
***               detected by format-4 reference).                ***
*** INPUT ARGS  : name  - symbol name                             ***
***               mflag - new MFLAG value                         ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : bool - false if symbol not found                ***
********************************************************************/
bool SymbolTable::setMFlag(const std::string& name, bool mflag) {
    std::string key = key6(name);
    auto it = pimpl->symbols.find(key);
    if (it == pimpl->symbols.end()) return false;
    it->second.mflag = mflag;
    return true;
}

/********************************************************************
*** FUNCTION setValueString                                       ***
*********************************************************************
*** DESCRIPTION : Overrides the printable VALUE string for an     ***
***               existing symbol (useful for EQU printing).      ***
*** INPUT ARGS  : name     - symbol name                          ***
***               valueStr - new printable value string           ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : bool - false if symbol not found                ***
********************************************************************/
bool SymbolTable::setValueString(const std::string& name, const std::string& valueStr) {
    std::string key = key6(name);
    auto it = pimpl->symbols.find(key);
    if (it == pimpl->symbols.end()) return false;
    it->second.valueStr = valueStr;
    return true;
}

/********************************************************************
*** FUNCTION setValueInt                                          ***
*********************************************************************
*** DESCRIPTION : Updates the numeric value for an existing       ***
***               symbol.                                         ***
*** INPUT ARGS  : name  - symbol name                             ***
***               value - new numeric value                       ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : bool - false if symbol not found                ***
********************************************************************/
bool SymbolTable::setValueInt(const std::string& name, int value) {
    std::string key = key6(name);
    auto it = pimpl->symbols.find(key);
    if (it == pimpl->symbols.end()) return false;
    it->second.value = value;
    return true;
}

/********************************************************************
*** FUNCTION setFlags                                             ***
*********************************************************************
*** DESCRIPTION : Sets all three flags (RFLAG, IFLAG, MFLAG) for  ***
***               an existing symbol at once.                     ***
*** INPUT ARGS  : name  - symbol name                             ***
***               rflag - relative flag                           ***
***               iflag - interpretation flag                     ***
***               mflag - modification flag                       ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : bool - false if symbol not found                ***
********************************************************************/
bool SymbolTable::setFlags(const std::string& name, bool rflag, bool iflag, bool mflag) {
    std::string key = key6(name);
    auto it = pimpl->symbols.find(key);
    if (it == pimpl->symbols.end()) return false;
    it->second.rflag = rflag;
    it->second.iflag = iflag;
    it->second.mflag = mflag;
    return true;
}

/********************************************************************
*** FUNCTION exists                                               ***
*********************************************************************
*** DESCRIPTION : Checks if a symbol is present in the table.     ***
*** INPUT ARGS  : name - symbol name                              ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : bool - true if present; false otherwise         ***
********************************************************************/
bool SymbolTable::exists(const std::string& name) const {
    return pimpl->symbols.find(key6(name)) != pimpl->symbols.end();
}

/********************************************************************
*** FUNCTION getAddress                                           ***
*********************************************************************
*** DESCRIPTION : Retrieves the numeric value (LOCCTR or EQU      ***
***               result) for a symbol.                           ***
*** INPUT ARGS  : name - symbol name                              ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : int - numeric value; -1 if not found            ***
********************************************************************/
int SymbolTable::getAddress(const std::string& name) const {
    auto it = pimpl->symbols.find(key6(name));
    if (it != pimpl->symbols.end()) return it->second.value;
    return -1;
}

/********************************************************************
*** FUNCTION isRelative                                           ***
*********************************************************************
*** DESCRIPTION : Retrieves the RFLAG (relative flag) for a       ***
***               symbol.                                         ***
*** INPUT ARGS  : name - symbol name                              ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : bool - true if relative; false if absolute or   ***
***               not found                                       ***
********************************************************************/
bool SymbolTable::isRelative(const std::string& name) const {
    auto it = pimpl->symbols.find(key6(name));
    if (it != pimpl->symbols.end()) return it->second.rflag;
    return false;
}

/********************************************************************
*** FUNCTION display                                              ***
*********************************************************************
*** DESCRIPTION : Prints the symbol table to stdout in columnar   ***
***               format: LABEL, VALUE, RFLAG, IFLAG, MFLAG.      ***
***               VALUE is printed as uppercase hex (no 0x) or    ***
***               explicit string if set. Symbols sorted          ***
***               alphabetically.                                 ***
*** INPUT ARGS  : none                                            ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : void                                            ***
********************************************************************/
void SymbolTable::display() const {
    std::cout << "\nSymbol Table\n";
    std::cout << std::left
              << std::setw(10) << "LABEL"
              << std::setw(8)  << "VALUE"
              << std::setw(7)  << "RFLAG"
              << std::setw(7)  << "IFLAG"
              << std::setw(7)  << "MFLAG" << "\n";

    std::vector<std::string> names;
    for (auto &p : pimpl->symbols) names.push_back(p.first);
    std::sort(names.begin(), names.end());

    for (auto &name : names) {
        const auto &sym = pimpl->symbols.at(name);
        std::string valueOut;
        if (!sym.valueStr.empty()) {
            valueOut = sym.valueStr;         // already a printable hex string (e.g., "200")
        } else {
            std::ostringstream oss;
            oss << std::uppercase << std::hex << sym.value;
            valueOut = oss.str();            // uppercase hex without 0x
        }
        std::cout << std::left
                  << std::setw(10) << name
                  << std::setw(8)  << valueOut
                  << std::setw(7)  << (sym.rflag ? 1 : 0)
                  << std::setw(7)  << (sym.iflag ? 1 : 0)
                  << std::setw(7)  << (sym.mflag ? 1 : 0)
                  << "\n";
    }
}