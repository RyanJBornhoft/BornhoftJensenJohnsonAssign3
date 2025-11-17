#include "LiteralTable.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <string>
#include <vector>
#include <map>

// toUpper helper
static std::string toUpper(const std::string& s) {
    std::string r = s;
    for (char &c : r) c = std::toupper((unsigned char)c);
    return r;
}

// Insert (returns true if newly inserted)
bool LiteralTable::insert(const std::string& literal) {
    auto it = literals.find(literal);
    if (it != literals.end()) return false;
    Literal lit;
    lit.raw = literal;
    literals[literal] = lit;
    return true;
}

// Assign addresses and compute hex/value/length for unassigned literals
int LiteralTable::assignAddresses(int startAddress) {
    int currentAddress = startAddress;
    for (auto &kv : literals) {
        auto &lit = kv.second;
        if (lit.assigned) continue;

        lit.address = currentAddress;
        lit.assigned = true;

        const std::string &raw = lit.raw;
        if (raw.size() >= 3 && (raw[1] == 'C' || raw[1] == 'c')) {
            size_t first = raw.find('\'');
            size_t last  = raw.rfind('\'');
            if (first != std::string::npos && last != std::string::npos && last > first) {
                std::string chars = raw.substr(first+1, last-first-1);
                std::ostringstream oss;
                for (unsigned char ch : chars)
                    oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << int(ch);
                lit.hexValue = toUpper(oss.str());
                lit.length = (int)chars.size();
            }
        } else if (raw.size() >= 3 && (raw[1] == 'X' || raw[1] == 'x')) {
            size_t first = raw.find('\'');
            size_t last  = raw.rfind('\'');
            if (first != std::string::npos && last != std::string::npos && last > first) {
                std::string hexpart = raw.substr(first+1, last-first-1);
                std::string cleaned;
                for (char c : hexpart) if (!std::isspace((unsigned char)c)) cleaned += c;
                lit.hexValue = toUpper(cleaned);
                lit.length = (int)((cleaned.length() + 1) / 2);
            }
        } else {
            lit.hexValue = raw;
            lit.length = 0;
        }
        currentAddress += lit.length;
    }
    return currentAddress;
}

// Parse literal (VALUE + LENGTH)
static bool parseLiteralValue(const std::string &literal,
                              std::string &hexValue,
                              int &byteLen) {
    hexValue.clear();
    byteLen = 0;
    if (literal.size() < 4 || literal[0] != '=') return false;
    char kind = std::toupper((unsigned char)literal[1]);
    size_t first = literal.find('\'');
    size_t last  = literal.rfind('\'');
    if (first == std::string::npos || last == std::string::npos || last <= first) return false;
    std::string body = literal.substr(first+1, last-first-1);

    if (kind == 'C') {
        std::ostringstream oss;
        for (unsigned char ch : body)
            oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << int(ch);
        hexValue = oss.str();
        byteLen = (int)body.size();
        return true;
    } else if (kind == 'X') {
        std::string hex = body;
        for (char &c : hex) c = std::toupper((unsigned char)c);
        hexValue = hex;
        byteLen = (int)((hex.size() + 1) / 2);
        return true;
    }
    return false;
}

// Display literal table
void LiteralTable::display() const {
    using std::cout; using std::left; using std::right; using std::setw;

    const int W_LIT  = 16;
    const int W_VAL  = 16;
    const int W_LEN  = 8;
    const int W_ADDR = 10;

    cout << "\nLiteral Table\n";
    cout << "-----------------------------------------\n";
    cout << left  << setw(W_LIT)  << "LITERAL"
         << right << setw(W_VAL)  << "VALUE"
         << right << setw(W_LEN)  << "LENGTH"
         << right << setw(W_ADDR) << "ADDRESS" << "\n";
    cout << "-----------------------------------------\n";

    auto list = getAssignedLiterals(); // sorted by address
    for (auto &p : list) {
        const std::string &lit = p.first;
        int addr = p.second;

        std::string valueHex;
        int lengthBytes = 0;
        parseLiteralValue(lit, valueHex, lengthBytes);

        std::ostringstream addrHex;
        addrHex << std::uppercase << std::hex << std::setw(5) << std::setfill('0') << (addr & 0xFFFFF);

        cout << left  << setw(W_LIT)  << lit
             << right << setw(W_VAL)  << valueHex
             << right << setw(W_LEN)  << lengthBytes
             << right << setw(W_ADDR) << addrHex.str() << "\n";
    }
    cout << "-----------------------------------------\n";
}

// Get assigned literals (sorted by address)
std::vector<std::pair<std::string,int>> LiteralTable::getAssignedLiterals() const {
    std::vector<std::pair<std::string,int>> result;
    for (const auto &kv : literals)
        if (kv.second.assigned)
            result.emplace_back(kv.first, kv.second.address);
    std::sort(result.begin(), result.end(),
              [](const std::pair<std::string,int> &a,
                 const std::pair<std::string,int> &b) { return a.second < b.second; });
    return result;
}

bool LiteralTable::setAddress(const std::string& literal, int addr) {
    auto it = literals.find(literal);
    if (it == literals.end()) return false;
    it->second.address = addr;
    it->second.assigned = true;
    return true;
}