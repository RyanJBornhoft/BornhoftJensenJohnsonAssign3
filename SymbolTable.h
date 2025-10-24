#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <string>

class SymbolTableImpl;

class SymbolTable {
public:
    SymbolTable();
    ~SymbolTable();

    bool insert(const std::string& name, int value,
                bool rflag = true, bool iflag = true, bool mflag = false);
    bool insertWithValueString(const std::string& name, const std::string& valueStr,
                               bool rflag = true, bool iflag = true, bool mflag = false);

    // Setters for post-insert adjustments
    bool setMFlag(const std::string& name, bool mflag);
    bool setValueString(const std::string& name, const std::string& valueStr);
    bool setValueInt(const std::string& name, int value);
    bool setFlags(const std::string& name, bool rflag, bool iflag, bool mflag);

    void display() const;

    bool exists(const std::string& name) const;
    int  getAddress(const std::string& name) const;   // numeric value (LOCCTR or EQU result)
    bool isRelative(const std::string& name) const;

private:
    SymbolTableImpl* pimpl;
};

#endif