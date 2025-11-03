#include "LiteralTable.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>
#include <cctype>

class LiteralTableImpl {
public:
    struct Literal {
        std::string raw;      // original literal text, e.g. "=C'ABCD'"
        std::string hexValue; // computed hex string, e.g. "41424344" or "FF"
        int length = 0;       // length in bytes
        int address = 0;      // assigned LOCCTR/address
        bool assigned = false;
    };
    std::map<std::string, Literal> literals;
};

LiteralTable::LiteralTable() : pimpl(new LiteralTableImpl()) {}

/********************************************************************
*** FUNCTION ~LiteralTable (destructor)                           ***
*********************************************************************
*** DESCRIPTION : Cleans up internal implementation object.       ***
*** INPUT ARGS  : none                                            ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : (destructor)                                    ***
********************************************************************/
LiteralTable::~LiteralTable() { delete pimpl; }

/********************************************************************
*** FUNCTION insert                                               ***
*********************************************************************
*** DESCRIPTION : Inserts a literal (e.g., =C'EOF' or =X'05')     ***
***               into the table if not already present. Does not ***
***               assign address yet.                             ***
*** INPUT ARGS  : literal - raw literal string                    ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : int - address if already assigned; -1 if newly  ***
***               inserted or unassigned                          ***
********************************************************************/
int LiteralTable::insert(const std::string& literal) {
    auto it = pimpl->literals.find(literal);
    if (it != pimpl->literals.end()) {
        return it->second.assigned ? it->second.address : -1;
    }
    LiteralTableImpl::Literal lit;
    lit.raw = literal;
    lit.hexValue = "";
    lit.length = 0;
    lit.address = 0;
    lit.assigned = false;
    pimpl->literals[literal] = lit;
    return -1;
}

static std::string toUpper(const std::string& s) {
    std::string r = s;
    for (char &c : r) c = std::toupper((unsigned char)c);
    return r;
}

/********************************************************************
*** FUNCTION assignAddresses                                      ***
*********************************************************************
*** DESCRIPTION : Assigns addresses to all unassigned literals    ***
***               starting at startAddress. Computes VALUE (hex)  ***
***               and LENGTH based on literal type (C'..' or      ***
***               X'..'). Updates LOCCTR by total literal bytes.  ***
*** INPUT ARGS  : startAddress - current LOCCTR to start placing  ***
***               literals                                        ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : int - new LOCCTR after all literals assigned    ***
********************************************************************/
int LiteralTable::assignAddresses(int startAddress) {
    int currentAddress = startAddress;

    // Assign addresses and compute hexValue/length for unassigned literals
    for (auto &pair : pimpl->literals) {
        auto &literal = pair.second;
        if (!literal.assigned) {
            literal.address = currentAddress;
            literal.assigned = true;

            const std::string &raw = literal.raw;
            if (raw.size() >= 3 && (raw[1] == 'C' || raw[1] == 'c')) {
                // Character literal =C'...'
                size_t first = raw.find('\'');
                size_t last = raw.rfind('\'');
                if (first != std::string::npos && last != std::string::npos && last > first) {
                    std::string chars = raw.substr(first+1, last-first-1);
                    std::ostringstream oss;
                    for (unsigned char ch : chars) {
                        oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
                            << int(ch);
                    }
                    literal.hexValue = toUpper(oss.str());
                    literal.length = static_cast<int>(chars.size());
                } else {
                    literal.hexValue = "";
                    literal.length = 0;
                }
            } else if (raw.size() >= 3 && (raw[1] == 'X' || raw[1] == 'x')) {
                // Hex literal =X'...'
                size_t first = raw.find('\'');
                size_t last = raw.rfind('\'');
                if (first != std::string::npos && last != std::string::npos && last > first) {
                    std::string hexpart = raw.substr(first+1, last-first-1);
                    // remove whitespace just in case
                    std::string cleaned;
                    for (char c : hexpart) if (!std::isspace((unsigned char)c)) cleaned += c;
                    literal.hexValue = toUpper(cleaned);
                    literal.length = static_cast<int>((cleaned.length() + 1) / 2);
                } else {
                    literal.hexValue = "";
                    literal.length = 0;
                }
            } else {
                // Unknown literal type: store raw and set length 0
                literal.hexValue = literal.raw;
                literal.length = 0;
            }

            currentAddress += literal.length;
        }
    }

    return currentAddress;
}

/********************************************************************
*** FUNCTION display                                              ***
*********************************************************************
*** DESCRIPTION : Prints the literal table to stdout in columnar  ***
***               format: LITERAL, VALUE, LENGTH, ADDRESS.        ***
***               VALUE is uppercase hex (no 0x). Sorted by       ***
***               literal key.                                    ***
*** INPUT ARGS  : none                                            ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : void                                            ***
********************************************************************/
void LiteralTable::display() const {
    using std::cout; using std::left; using std::right; using std::setw;

    // Use the same widths for header and rows
    const int W_LIT  = 16;
    const int W_VAL  = 16;
    const int W_LEN  = 8;
    const int W_ADDR = 10;

    cout << "\nLiteral Table\n";
    cout << "-----------------------------------------\n";
    cout << left  << setw(W_LIT)  << "LITERAL"
         << left << setw(W_VAL)  << "VALUE"
         << left << setw(W_LEN)  << "LENGTH"
         << right << setw(W_ADDR) << "ADDRESS" << "\n";
    cout << "-----------------------------------------\n";

    // sort keys for stable output
    std::vector<std::string> keys;
    for (const auto &p : pimpl->literals) keys.push_back(p.first);
    std::sort(keys.begin(), keys.end());

    for (const auto &k : keys) {
        const auto &lit = pimpl->literals.at(k);
        std::cout << std::left << std::setw(W_LIT) << k
                  << std::setw(W_VAL) << (lit.hexValue.empty() ? lit.raw : lit.hexValue)
                  << std::setw(W_LEN)  << lit.length;

        // ADDRESS as 5-digit uppercase hex (program-relative)
        std::ostringstream addrHex;
        addrHex << std::uppercase << std::hex
                << std::setw(5) << std::setfill('0')
                << (lit.address & 0xFFFFF);
        cout << std::right << std::setw(W_ADDR) << addrHex.str() << "\n";

        // restore defaults if you modify stream state elsewhere
        cout << std::dec << std::setfill(' ');
    }

    cout << "-----------------------------------------\n";
}

/********************************************************************
*** FUNCTION getAssignedLiterals                                  ***
*********************************************************************
*** DESCRIPTION : Returns a list of all assigned literals with    ***
***               their addresses for intermediate listing.       ***
*** INPUT ARGS  : none                                            ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : vector<pair<string,int>> - literal name and     ***
***               assigned address                                ***
********************************************************************/
std::vector<std::pair<std::string, int>> LiteralTable::getAssignedLiterals() const {
    std::vector<std::pair<std::string, int>> result;
    for (const auto &p : pimpl->literals) {
        if (p.second.assigned) {
            result.push_back({p.first, p.second.address});
        }
    }
    // Sort by address for consistent listing order (C++11 compatible lambda)
    std::sort(result.begin(), result.end(),
              [](const std::pair<std::string, int> &a, const std::pair<std::string, int> &b) {
                  return a.second < b.second;
              });
    return result;
}