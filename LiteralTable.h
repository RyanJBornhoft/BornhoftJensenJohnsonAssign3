#pragma once

#include <string>
#include <vector>
#include <map>

class LiteralTable {
public:
    bool insert(const std::string& literal);
    int  assignAddresses(int startAddress);
    std::vector<std::pair<std::string,int>> getAssignedLiterals() const;
    void display() const;
    bool setAddress(const std::string& literal, int addr);
private:
    struct Literal {
        std::string raw;
        std::string hexValue;
        int length = 0;
        int address = 0;
        bool assigned = false;
    };
    std::map<std::string, Literal> literals;
};