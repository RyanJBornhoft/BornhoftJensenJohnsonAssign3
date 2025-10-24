#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <map>
#include "SymbolTable.h"
#include "LiteralTable.h"
#include "OpcodeTable.h"

using namespace std;

/********************************************************************
*** FUNCTION trim                                                 ***
*********************************************************************
*** DESCRIPTION : Removes leading and trailing whitespace from    ***
***               a string.                                        ***
*** INPUT ARGS  : str - the input string to trim                  ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : string - trimmed copy of input                  ***
********************************************************************/
string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

/********************************************************************
*** FUNCTION toUpper                                              ***
*********************************************************************
*** DESCRIPTION : Converts all lowercase letters in a string to   ***
***               uppercase (ASCII).                              ***
*** INPUT ARGS  : str - the string to convert (by value)          ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : string - uppercased copy                        ***
********************************************************************/
string toUpper(string str) {
    transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

/********************************************************************
*** FUNCTION isValidSymbol                                        ***
*********************************************************************
*** DESCRIPTION : Validates a SIC/XE symbol: 1-6 chars, starts    ***
***               with alphabetic, remaining alphanumeric.        ***
*** INPUT ARGS  : str - candidate symbol string                   ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : bool - true if valid, false otherwise           ***
********************************************************************/
bool isValidSymbol(const string& str) {
    if (str.empty() || str.length() > 6) return false;
    if (!isalpha(str[0])) return false;
    
    for (char c : str) {
        if (!isalnum(c)) return false;
    }
    return true;
}

/********************************************************************
*** STRUCT ParsedLine                                             ***
*********************************************************************
*** DESCRIPTION : Holds the parsed components of a source line:   ***
***               label, opcode, operand, comment. isComment=true ***
***               for full-line comments starting with '.'.       ***
********************************************************************/
struct ParsedLine {
    string label;
    string opcode;
    string operand;
    string comment;
    bool isComment;
};

/********************************************************************
*** FUNCTION parseLine                                            ***
*********************************************************************
*** DESCRIPTION : Tokenizes a source line into label, opcode,     ***
***               operand, and comment. Detects full-line and     ***
***               inline comments (starting with '.').            ***
*** INPUT ARGS  : line - raw source line string                   ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : ParsedLine - struct with fields: label, opcode, ***
***               operand, comment, isComment                     ***
********************************************************************/
ParsedLine parseLine(const string& line) {
    ParsedLine parsed;
    parsed.isComment = false;
    
    // Check for full-line comment
    if (line.empty() || line[0] == '.') {
        parsed.isComment = true;
        parsed.comment = line;
        return parsed;
    }
    
    // Find inline comment
    size_t commentPos = line.find('.');
    string codePart = (commentPos != string::npos) ? line.substr(0, commentPos) : line;
    parsed.comment = (commentPos != string::npos) ? line.substr(commentPos) : "";
    
    istringstream iss(codePart);
    vector<string> tokens;
    string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    if (tokens.empty()) {
        parsed.isComment = true;
        return parsed;
    }
    
    // Determine if first token is a label
    // Label starts in column 0 (no leading whitespace in original line)
    bool hasLabel = !line.empty() && line[0] != ' ' && line[0] != '\t';
    
    size_t idx = 0;
    if (hasLabel && tokens.size() > 0) {
        parsed.label = tokens[0];
        idx = 1;
    }
    
    if (idx < tokens.size()) {
        parsed.opcode = toUpper(tokens[idx]);
        idx++;
    }
    
    if (idx < tokens.size()) {
        parsed.operand = tokens[idx];
        idx++;
    }
    
    if (idx < tokens.size()) {
        // join remaining tokens into comment/extra operand
        for (size_t i = idx; i < tokens.size(); ++i) {
            if (i > idx) parsed.operand += " ";
            parsed.operand += tokens[i];
        }
    }
    
    return parsed;
}

/********************************************************************
*** FUNCTION getInstructionLength                                 ***
*********************************************************************
*** DESCRIPTION : Computes the byte length for an instruction or  ***
***               directive. Handles format 4 (+OP), WORD, RESW,  ***
***               RESB, BYTE (C'..' and X'..'), and opcode table  ***
***               formats.                                        ***
*** INPUT ARGS  : opcode  - operation or directive string         ***
***               operand - operand string (for BYTE/RES*)        ***
***               optab   - reference to opcode table             ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : int - length in bytes; 0 if unknown             ***
********************************************************************/
int getInstructionLength(const string& opcode, const string& operand, OpcodeTable& optab) {
    string op = opcode;
    
    // Check for format 4 (starts with +)
    if (!op.empty() && op[0] == '+') {
        return 4;
    }
    
    // Check directives
    if (op == "WORD") return 3;
    if (op == "RESW") return 3 * stoi(operand);
    if (op == "RESB") return stoi(operand);
    if (op == "BYTE") {
        // C'...' or X'...'
        if (operand[0] == 'C' || operand[0] == 'c') {
            size_t start = operand.find('\'');
            size_t end = operand.rfind('\'');
            return end - start - 1;
        } else if (operand[0] == 'X' || operand[0] == 'x') {
            size_t start = operand.find('\'');
            size_t end = operand.rfind('\'');
            return (end - start - 1 + 1) / 2;
        }
        return 1;
    }
    
    // Check opcodes
    if (optab.exists(op)) {
        int format = optab.getFormat(op);
        return format;
    }
    
    return 0;
}

/********************************************************************
*** FUNCTION evaluateExpression                                   ***
*********************************************************************
*** DESCRIPTION : Parses a simple integer expression (decimal or  ***
***               hex). Supports forms: 123, $FFFF, 0xFFFF.       ***
*** INPUT ARGS  : expr - expression string                        ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : int - evaluated integer value                   ***
********************************************************************/
int evaluateExpression(const string& expr) {
    string e = trim(expr);
    if (e.empty()) return 0;
    
    // Check for hex
    if (e[0] == '$' || (e.length() > 2 && e[0] == '0' && (e[1] == 'x' || e[1] == 'X'))) {
        return stoi(e.substr(e[0] == '$' ? 1 : 2), nullptr, 16);
    }
    
    // Decimal
    return stoi(e);
}

/********************************************************************
*** FUNCTION writeIntermediateHeader                              ***
*********************************************************************
*** DESCRIPTION : Writes column headers for the intermediate      ***
***               listing: LINE#, LOCCTR, LABEL, OPERATION,       ***
***               OPERAND.                                        ***
*** INPUT ARGS  : outFile - reference to open output stream       ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : void                                            ***
********************************************************************/
void writeIntermediateHeader(std::ofstream& outFile) {
    outFile << "LINE#  LOCCTR    LABEL      OPERATION   OPERAND\n";
}

/********************************************************************
*** FUNCTION writeLine                                            ***
*********************************************************************
*** DESCRIPTION : Writes a single intermediate listing row with   ***
***               formatted columns: 2-digit LINE#, 5-hex LOCCTR, ***
***               LABEL (with ':'), OPERATION, OPERAND.           ***
*** INPUT ARGS  : outFile  - reference to open output stream      ***
***               lineNum  - 1-based source line number           ***
***               locctr   - program-relative location counter    ***
***               label    - label string (may be empty or "*")   ***
***               opcode   - operation/directive string           ***
***               operand  - operand string (may be empty)        ***
***               optab    - reference to opcode table (reserved) ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : void                                            ***
********************************************************************/
void writeLine(std::ofstream& outFile, int lineNum, int locctr,
               const std::string& label, const std::string& opcode,
               const std::string& operand, const OpcodeTable &optab) {
    (void)optab; // not used here

    // LINE# as 2 digits with leading zero
    outFile << std::right << std::setw(2) << std::setfill('0') << lineNum
            << std::setfill(' ') << "     ";

    // LOCCTR as 5 hex digits uppercase
    outFile << std::uppercase << std::hex << std::setw(5) << std::setfill('0')
            << (locctr & 0xFFFFF) << std::dec << std::setfill(' ')
            << std::nouppercase << "   ";

    // LABEL with ':' suffix (or "*" for literals)
    std::string labelOut;
    if (!label.empty()) {
        labelOut = (label == "*") ? "*" : (label + ":");
    }
    outFile << std::left << std::setw(11) << labelOut;

    // OPERATION and OPERAND
    outFile << std::left << std::setw(12) << opcode
            << operand << "\n";
}

static bool isNumber(const std::string& s) {
    if (s.empty()) return false;
    for (char c : s) if (!isdigit((unsigned char)c)) return false;
    return true;
}

/********************************************************************
*** STRUCT EquEval                                                ***
*********************************************************************
*** DESCRIPTION : Holds the result of evaluating an EQU operand:  ***
***               numeric value, rflag, and success flag.         ***
********************************************************************/
struct EquEval { int value = 0; bool rflag = false; bool ok = true; };

/********************************************************************
*** FUNCTION evalEQU                                              ***
*********************************************************************
*** DESCRIPTION : Evaluates a simple EQU operand expression:      ***
***               numeric constant, single symbol, or SYM-SYM.    ***
***               Computes value and rflag (relative if R-A;      ***
***               absolute if R-R or A-A).                        ***
*** INPUT ARGS  : expr   - EQU operand string                     ***
***               symtab - reference to symbol table              ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : EquEval - struct with value, rflag, ok fields   ***
********************************************************************/
static EquEval evalEQU(const std::string& expr, const SymbolTable& symtab) {
    std::string e = trim(expr);
    EquEval r;
    // SYM-SYM
    auto minus = e.find('-');
    if (minus != std::string::npos) {
        std::string a = trim(e.substr(0, minus));
        std::string b = trim(e.substr(minus + 1));
        int va = isNumber(a) ? stoi(a) : symtab.getAddress(a);
        int vb = isNumber(b) ? stoi(b) : symtab.getAddress(b);
        bool ra = isNumber(a) ? false : symtab.isRelative(a);
        bool rb = isNumber(b) ? false : symtab.isRelative(b);
        // R - R => absolute; R - A => relative; A - R => error; A - A => absolute
        if (!isNumber(a) && va < 0) { r.ok = false; return r; }
        if (!isNumber(b) && vb < 0) { r.ok = false; return r; }
        if (!ra && rb) { r.ok = false; return r; } // A - R invalid
        r.value = va - vb;
        r.rflag = (ra && !rb); // relative if R - A
        return r;
    }
    // number
    if (isNumber(e)) { r.value = stoi(e); r.rflag = false; return r; }
    // single symbol
    int v = symtab.getAddress(e);
    if (v < 0) { r.ok = false; return r; }
    r.value = v;
    r.rflag = symtab.isRelative(e);
    return r;
}

/********************************************************************
*** FUNCTION main                                                 ***
*********************************************************************
*** DESCRIPTION : Entry point for SIC/XE Pass 1. Reads source     ***
***               file, parses lines, maintains LOCCTR, builds    ***
***               symbol and literal tables, emits intermediate   ***
***               listing, and prints summary tables.             ***
*** INPUT ARGS  : argc - argument count                           ***
***               argv - argument vector (argv[1] is filename)    ***
*** OUTPUT ARGS : none                                            ***
*** IN/OUT ARGS : none                                            ***
*** RETURN      : int - 0 on success; non-zero on errors          ***
********************************************************************/
int main(int argc, char* argv[]) {
    string filename;
    
    // Get filename from command line or prompt
    if (argc > 1) {
        filename = argv[1];
    } else {
        cout << "Enter source file name: ";
        getline(cin, filename);
    }
    
    // Open source file
    ifstream sourceFile(filename);
    if (!sourceFile.is_open()) {
        cerr << "Error: Cannot open file " << filename << endl;
        return 1;
    }
    
    // Determine output filenames
    string baseName = filename.substr(0, filename.find_last_of('.'));
    string intFilename = baseName + ".int";

    // Open intermediate file for output (needed by writeLine)
    std::ofstream intermediateFile(intFilename);
    if (!intermediateFile.is_open()) {
        cerr << "Error: Cannot open intermediate file " << intFilename << endl;
        return 1;
    }
    // write intermediate header
    writeIntermediateHeader(intermediateFile);

    // Initialize tables
    SymbolTable symtab;
    LiteralTable littab;
    OpcodeTable optab;
    
    // Pass 1 variables
    int LOCCTR = 0;
    int startAddress = 0;
    string programName;
    int programLength = 0;
    vector<string> sourceLines;
    vector<int> lineAddresses;
    vector<ParsedLine> parsedLines;
    // pending modification flags for symbols referenced by format-4 before symbol is defined
    std::map<std::string, bool> pendingMFlags;
    
    bool errorCheckingEnabled = true; // Set to false to disable error checking
    bool hasError = false;
    
    cout << "\n========== PASS 1 - SIC/XE ASSEMBLER ==========" << endl;
    cout << "Processing file: " << filename << endl;
    
    // Read all lines first
    string line;
    int lineNumber = 0;
    
    while (getline(sourceFile, line)) {
        sourceLines.push_back(line);
        lineNumber++;
        
        ParsedLine parsed = parseLine(line);
        parsedLines.push_back(parsed);
        
        // Skip comments
        if (parsed.isComment) {
            lineAddresses.push_back(-1);
            continue;
        }
        
        // Insert label for non-EQU lines at current LOCCTR (program-relative)
        if (!parsed.label.empty() && parsed.opcode != "EQU" && parsed.opcode != "BASE") {
            if (!symtab.insert(parsed.label, LOCCTR, true, true, false)) {
                std::cerr << "Error: Duplicate symbol '" << parsed.label
                          << "' on line " << lineNumber << std::endl;
                hasError = true;
            }
        }
        
        // Handle START: keep LOCCTR relative (0)
        if (parsed.opcode == "START" && LOCCTR == 0) {
            startAddress = evaluateExpression(parsed.operand);
            LOCCTR = 0; // program-relative
            lineAddresses.push_back(LOCCTR);
            writeLine(intermediateFile, lineNumber, LOCCTR, parsed.label, parsed.opcode, parsed.operand, optab);
            continue;
        }
        
        // Handle EQU
        if (parsed.opcode == "EQU") {
            EquEval eq = evalEQU(parsed.operand, symtab);
            // Printable VALUE (uppercase hex without 0x) per example
            std::ostringstream oss; oss << std::uppercase << std::hex << (eq.value & 0xFFFF);
            std::string valueHex = oss.str();

            if (!parsed.label.empty()) {
                if (!symtab.exists(parsed.label)) {
                    symtab.insert(parsed.label, eq.value, eq.rflag, true, false);
                } else {
                    symtab.setValueInt(parsed.label, eq.value);
                    symtab.setFlags(parsed.label, eq.rflag, true, false);
                }
                symtab.setValueString(parsed.label, valueHex);
            }
            // EQU does not advance LOCCTR
            lineAddresses.push_back(LOCCTR);
            writeLine(intermediateFile, lineNumber, LOCCTR, parsed.label, parsed.opcode, parsed.operand, optab);
            continue;
        }
        
        // Check for literals in operand
        if (!parsed.operand.empty() && parsed.operand[0] == '=') {
            littab.insert(parsed.operand);
        }
        
        // Handle directives
        if (parsed.opcode == "END") {
            // Assign addresses to any remaining literals
            LOCCTR = littab.assignAddresses(LOCCTR);
            programLength = LOCCTR - startAddress;
            break;
        }
        
        if (parsed.opcode == "LTORG") {
            lineAddresses.push_back(LOCCTR);
            writeLine(intermediateFile, lineNumber, LOCCTR, parsed.label, parsed.opcode, parsed.operand, optab);
            
            // Assign literal addresses and write them to intermediate file
            LOCCTR = littab.assignAddresses(LOCCTR);
            
            // Write assigned literals
            auto lits = littab.getAssignedLiterals();
            for (const auto &lit : lits) {
                lineNumber++;
                writeLine(intermediateFile, lineNumber, lit.second, "*", lit.first, "", optab);
                lineAddresses.push_back(lit.second);
            }
            
            programLength = LOCCTR;
            continue;
        }

        // Handle END directive
        if (parsed.opcode == "END") {
            lineAddresses.push_back(LOCCTR);
            writeLine(intermediateFile, lineNumber, LOCCTR, parsed.label, parsed.opcode, parsed.operand, optab);
            
            // Assign remaining literals and write them
            LOCCTR = littab.assignAddresses(LOCCTR);
            
            // Write assigned literals
            auto lits = littab.getAssignedLiterals();
            for (const auto &lit : lits) {
                lineNumber++;
                writeLine(intermediateFile, lineNumber, lit.second, "*", lit.first, "", optab);
                lineAddresses.push_back(lit.second);
            }
            
            programLength = LOCCTR;
            break;
        }
        
        if (parsed.opcode == "EQU") {
            // Handle EQU - symbol already added with current LOCCTR
            // Would need expression evaluation for full support
            continue;
        }
        
        if (parsed.opcode == "BASE" || parsed.opcode == "NOBASE") {
            continue; // No address increment
        }
        
        // Calculate length and increment LOCCTR
        if (errorCheckingEnabled && !optab.exists(parsed.opcode) && 
            parsed.opcode != "WORD" && parsed.opcode != "RESW" && 
            parsed.opcode != "RESB" && parsed.opcode != "BYTE" &&
            parsed.opcode != "START" && parsed.opcode != "END" && 
            parsed.opcode != "BASE" && parsed.opcode != "NOBASE" &&
            parsed.opcode != "LTORG" && parsed.opcode != "EQU" &&
            parsed.opcode != "EXTDEF" && parsed.opcode != "EXTREF") {
            cerr << "Line " << lineNumber << ": Illegal instruction '" 
                 << parsed.opcode << "'" << endl;
            hasError = true;
        }
        
        int length = getInstructionLength(parsed.opcode, parsed.operand, optab);
        LOCCTR += length;
    }
    
    sourceFile.close();
    
    // Write intermediate file
    ofstream intFile(intFilename);
    if (!intFile.is_open()) {
        cerr << "Error: Cannot create intermediate file" << endl;
        return 1;
    }
    
    cout << "\n========== INTERMEDIATE FILE ==========" << endl;
    cout << left << setw(6) << "Line" << setw(10) << "LOCCTR" 
         << "Label" << "  Statement" << endl;
    cout << string(70, '-') << endl;
    
    writeIntermediateHeader(intFile);
    
    for (size_t i = 0; i < sourceLines.size(); i++) {
        string addrStr;
        if (lineAddresses[i] == -1) {
            addrStr = "";
        } else {
            stringstream ss;
            ss << hex << uppercase << setfill('0') << setw(4) << lineAddresses[i];
            addrStr = ss.str();
        }
        
        // Write to intermediate file
        intFile << setw(6) << (i + 1) 
                << setw(10) << addrStr 
                << sourceLines[i] << endl;
        
        // Write to screen
        cout << setw(6) << (i + 1) 
             << setw(10) << addrStr 
             << sourceLines[i] << endl;
    }
    
    intFile.close();
    
    cout << "\nIntermediate file written to: " << intFilename << endl;
    cout << "\nProgram Name: " << programName << endl;
    cout << "Start Address: " << hex << uppercase << startAddress << endl;
    cout << "Program Length: " << programLength << dec << " bytes" << endl;
    
    // Display tables
    symtab.display();
    littab.display();
    
    if (errorCheckingEnabled) {
        if (hasError) {
            cout << "\n*** ERRORS DETECTED - See messages above ***" << endl;
        } else {
            cout << "\n*** No errors detected ***" << endl;
        }
    }
    
    cout << "\n========== PASS 1 COMPLETE ==========" << endl;
    
    intermediateFile.close();
    return 0;
}