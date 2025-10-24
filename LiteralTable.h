#ifndef LITERAL_TABLE_H
#define LITERAL_TABLE_H

#include <string>
#include <vector>

// Forward declaration
class LiteralTableImpl;

class LiteralTable {
public:
    LiteralTable();
    ~LiteralTable();
    
    // Prevent copying
    LiteralTable(const LiteralTable&) = delete;
    LiteralTable& operator=(const LiteralTable&) = delete;
    
    int insert(const std::string& literal);
    int assignAddresses(int startAddress);
    void display() const;
    
    /**
     * @brief Get all literals for listing purposes.
     * @return Vector of pairs: (literal name, address).
     */
    std::vector<std::pair<std::string, int>> getAssignedLiterals() const;

private:
    LiteralTableImpl* pimpl;  // PIMPL idiom
};

#endif