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

/* Add numeric check used by evalEQU */
static bool isNumber(const std::string &s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
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
*** DESCRIPTION : Writes the fixed header line to the .int file.  ***
*** INPUT ARGS  : outFile - open intermediate stream              ***
*** RETURN      : void                                             ***
********************************************************************/
void writeIntermediateHeader(std::ofstream& outFile) {
    outFile << "LINE#  LOCCTR    LABEL      OPERATION   OPERAND\n";
}

/********************************************************************
*** FUNCTION formatLabelForIntermediate                            ***
*********************************************************************
*** DESCRIPTION : Normalize label text for .int output: single    ***
***               trailing colon for symbols, "*" preserved.      ***
*** INPUT ARGS  : label - parsed label (may include colon)        ***
*** RETURN      : string - formatted label                        ***
********************************************************************/
static std::string formatLabelForIntermediate(const std::string &label) {
    if (label.empty()) return "";
    if (label == "*") return "*";
    std::string base = label;
    if (!base.empty() && base.back() == ':') base.pop_back();
    return base + ":";
}

/********************************************************************
*** FUNCTION writeLine                                            ***
*********************************************************************
*** DESCRIPTION : Writes one formatted intermediate listing row.  ***
***               Uses 2-digit LINE#, 5-hex LOCCTR, fixed columns.***
*** INPUT ARGS  : outFile, lineNum, locctr, label, opcode, operand***
*** RETURN      : void                                             ***
********************************************************************/
void writeLine(std::ofstream& outFile, int lineNum, int locctr,
               const std::string& label, const std::string& opcode,
               const std::string& operand, const OpcodeTable &optab) {
    (void)optab;
    // LINE#
    outFile << std::right << std::setw(2) << std::setfill('0') << lineNum;
    outFile << std::setfill(' ') << "     ";

    // LOCCTR as 5 uppercase hex
    std::ostringstream locoss;
    locoss << std::uppercase << std::hex << std::setw(5) << std::setfill('0')
           << (locctr & 0xFFFFF);
    outFile << locoss.str() << "   ";

    // LABEL (normalized)
    std::string lab = formatLabelForIntermediate(label);
    outFile << std::left << std::setw(11) << lab;

    // OPERATION and OPERAND
    outFile << std::left << std::setw(12) << opcode << operand << "\n";

    // restore i/o flags just in case
    outFile << std::dec;
}

/********************************************************************
*** FUNCTION writeLiteralDump                                      ***
*********************************************************************
*** DESCRIPTION : Append assigned literals at the end of the .int ***
***               file. Each literal gets its own incremented line.***
*** INPUT ARGS  : outFile, lineNum (by ref), littab, optab        ***
*** RETURN      : void                                             ***
********************************************************************/
static void writeLiteralDump(std::ofstream &outFile, int &lineNum,
                             const LiteralTable &littab,
                             const OpcodeTable &optab) {
    std::vector<std::pair<std::string,int>> lits = littab.getAssignedLiterals();
    if (lits.empty()) return;

    // blank separator for readability
    outFile << "\n";
    for (const auto &p : lits) {
        ++lineNum;
        // write literal line: LOCCTR = assigned address, label = "*", operand = literal text
        writeLine(outFile, lineNum, p.second, "*", p.first, "", optab);
    }
}

/* --- Add EquEval and evalEQU helper (simple evaluator) --- */
struct EquEval { int value; bool rflag; bool ok; };

static EquEval evalEQU(const std::string &expr, const SymbolTable &symtab) {
    EquEval r; r.value = 0; r.rflag = false; r.ok = true;
    std::string e = trim(expr);
    if (e.empty()) { r.ok = false; return r; }

    // If "*" (current location) - we can't know LOCCTR here; return 0 and mark ok
    if (e == "*") { r.value = 0; r.rflag = false; return r; }

    // If simple subtraction of two operands: A-B
    size_t minus = e.find('-');
    if (minus != std::string::npos) {
        std::string a = trim(e.substr(0, minus));
        std::string b = trim(e.substr(minus + 1));
        int va = 0, vb = 0;
        if (isNumber(a)) va = std::stoi(a); else va = symtab.getAddress(a);
        if (isNumber(b)) vb = std::stoi(b); else vb = symtab.getAddress(b);
        r.value = va - vb;
        // rflag is relative if operand flags differ; approximate as false here
        r.rflag = false;
        return r;
    }

    // Single token: number or symbol
    if (isNumber(e)) {
        r.value = std::stoi(e);
        r.rflag = false;
        return r;
    } else {
        int addr = symtab.getAddress(e);
        if (addr >= 0) { r.value = addr; r.rflag = true; }
        else r.ok = false;
        return r;
    }
}

/* --- Define stripColon (was forward-declared) --- */
static std::string stripColon(const std::string &s) {
    if (s.empty()) return s;
    if (s.back() == ':') return s.substr(0, s.size() - 1);
    return s;
}

/* --- Disable duplicate intermediate header definition (second copy) --- */
#if 0
// duplicate writeIntermediateHeader (disabled)
void writeIntermediateHeader(std::ofstream& outFile) {
    outFile << "LINE#  LOCCTR    LABEL      OPERATION   OPERAND\n";
}
#endif

/********************************************************************
*** FUNCTION displayIntermediateFile                               ***
*********************************************************************
*** DESCRIPTION : Reads and prints the intermediate file to screen ***
*** INPUT ARGS  : filename - path to .int file                     ***
*** RETURN      : void                                             ***
********************************************************************/
static void displayIntermediateFile(const std::string &filename) {
    std::ifstream inFile(filename);
    if (!inFile.is_open()) {
        std::cerr << "Error: could not open intermediate file for display: " << filename << "\n";
        return;
    }
    std::cout << "\n========== INTERMEDIATE FILE ==========\n";
    std::string line;
    while (std::getline(inFile, line)) {
        std::cout << line << "\n";
    }
    std::cout << "========================================\n";
    inFile.close();
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
    int outLineNumber = 0;
    bool endEmitted = false; // track whether END was written into the .int

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

        // Insert label into symbol table (use LOCCTR, lineNumber, hasError)
        if (!parsed.label.empty()) {
            // store symbol name internally without trailing colon
            std::string symName = stripColon(parsed.label);
            // Don't insert BASE directive labels
            if (parsed.opcode != "BASE") {
                if (!symtab.insert(symName, LOCCTR, true, true, false)) {
                    std::cerr << "Error: Duplicate symbol '" << symName
                              << "' on line " << lineNumber << std::endl;
                    hasError = true;
                } else {
                    // If there was a pending MFLAG for this symbol, set it now
                    auto itpf = pendingMFlags.find(symName);
                    if (itpf != pendingMFlags.end() && itpf->second) {
                        symtab.setMFlag(symName, true);
                        pendingMFlags.erase(itpf);
                    }
                }
            }
        }

        // NOTE: when other code references symbol names (e.g. EQU handling,
        // setValueString/setValueInt, or any later symtab lookups), use
        // stripColon(parsed.label) to obtain the canonical symbol name.
        
        // Detect format-4 usage that requires modification record (MFLAG).
        // Keep this inside the line-processing loop so `parsed` is in scope.
        if (!parsed.opcode.empty() && parsed.opcode[0] == '+' && !parsed.operand.empty()) {
            std::string opnd = parsed.operand;
            // ignore immediate (#), indirect (@), and literal (=) operands
            if (opnd[0] != '#' && opnd[0] != '@' && opnd[0] != '=') {
                // strip indexing or trailing commas (e.g., "SYMBOL,X")
                size_t comma = opnd.find(',');
                std::string symname = (comma == std::string::npos) ? opnd : opnd.substr(0, comma);
                // trim whitespace just in case
                symname = trim(symname);
                // try to set MFLAG now; if symbol not yet present, record pending MFLAG
                // normalize any trailing colon if present (defensive)
                symname = stripColon(symname);
                if (!symtab.setMFlag(symname, true)) {
                    pendingMFlags[symname] = true;
                }
             }
         }
        
        // Handle START: keep LOCCTR relative (0)
        if (parsed.opcode == "START" && LOCCTR == 0) {
            startAddress = evaluateExpression(parsed.operand);
            LOCCTR = 0; // program-relative
            lineAddresses.push_back(LOCCTR);
            writeLine(intermediateFile, ++outLineNumber, LOCCTR, parsed.label, parsed.opcode, parsed.operand, optab);
            continue;
        }
        
        // Handle EQU
        if (parsed.opcode == "EQU") {
            std::string op = trim(parsed.operand);
            EquEval eq;
            if (op == "*") {
                // current location; relocatable
                eq.value = LOCCTR;
                eq.rflag = true;
                eq.ok = true;
            } else {
                eq = evalEQU(op, symtab);
            }
             // Printable VALUE (uppercase hex without 0x)
             std::ostringstream oss; oss << std::uppercase << std::hex << (eq.value & 0xFFFF);
             std::string valueHex = oss.str();

             if (!parsed.label.empty()) {
                 std::string symName = stripColon(parsed.label);
                 if (!symtab.exists(symName)) {
                     symtab.insert(symName, eq.value, eq.rflag, true, false);
                 } else {
                     symtab.setValueInt(symName, eq.value);
                     symtab.setFlags(symName, eq.rflag, true, false);
                 }
                 symtab.setValueString(symName, valueHex);
             }
             // EQU does not advance LOCCTR. In the listing, show the symbol's value
             // (eq.value) in the LOCCTR column rather than the current LOCCTR.
             int listingLoc = eq.ok ? eq.value : LOCCTR;
             lineAddresses.push_back(listingLoc);
             writeLine(intermediateFile, ++outLineNumber, listingLoc,
                       parsed.label, parsed.opcode, parsed.operand, optab);
             continue;
        }

        // Check for literals in operand
        if (!parsed.operand.empty() && parsed.operand[0] == '=') {
            littab.insert(parsed.operand);
        }
        
        // Handle directives
        if (parsed.opcode == "END") {
            // Emit the END line
            writeLine(intermediateFile, ++outLineNumber, LOCCTR, parsed.label, parsed.opcode, parsed.operand, optab);
            endEmitted = true;

            // Assign remaining literals and write them
            LOCCTR = littab.assignAddresses(LOCCTR);

            auto lits = littab.getAssignedLiterals();
            for (const auto &lit : lits) {
                writeLine(intermediateFile, ++outLineNumber, lit.second, "*", lit.first, "", optab);
                lineAddresses.push_back(lit.second);
            }

            programLength = LOCCTR;
            break;
        }
        
        if (parsed.opcode == "LTORG") {
            lineAddresses.push_back(LOCCTR);
            writeLine(intermediateFile, ++outLineNumber, LOCCTR, parsed.label, parsed.opcode, parsed.operand, optab);

            // Assign literal addresses and write them to intermediate file
            LOCCTR = littab.assignAddresses(LOCCTR);

            // Write assigned literals
            auto lits = littab.getAssignedLiterals();
            for (const auto &lit : lits) {
                writeLine(intermediateFile, ++outLineNumber, lit.second, "*", lit.first, "", optab);
                lineAddresses.push_back(lit.second);
            }

            programLength = LOCCTR;
            continue;
        }

        // Handle END directive
        if (parsed.opcode == "END") {
            // Emit the END line
            writeLine(intermediateFile, ++outLineNumber, LOCCTR, parsed.label, parsed.opcode, parsed.operand, optab);
            endEmitted = true;

            // Assign remaining literals and write them
            LOCCTR = littab.assignAddresses(LOCCTR);

            auto lits = littab.getAssignedLiterals();
            for (const auto &lit : lits) {
                writeLine(intermediateFile, ++outLineNumber, lit.second, "*", lit.first, "", optab);
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
        
        // For ordinary instructions/directives write a listing line and then advance LOCCTR
        int length = getInstructionLength(parsed.opcode, parsed.operand, optab);
        writeLine(intermediateFile, ++outLineNumber, LOCCTR, parsed.label, parsed.opcode, parsed.operand, optab);
        LOCCTR += length;
    }
    
    sourceFile.close();

    // After the main loop: if END was in the source but not emitted, write it now
    if (!endEmitted) {
        for (size_t i = 0; i < parsedLines.size(); ++i) {
            const ParsedLine &p = parsedLines[i];
            if (p.opcode == "END") {
                // use current LOCCTR (final location) for END line
                writeLine(intermediateFile, ++outLineNumber, LOCCTR, p.label, p.opcode, p.operand, optab);
                // assign & write any remaining literals (if not already)
                LOCCTR = littab.assignAddresses(LOCCTR);
                auto lits = littab.getAssignedLiterals();
                for (const auto &lit : lits) {
                    writeLine(intermediateFile, ++outLineNumber, lit.second, "*", lit.first, "", optab);
                    lineAddresses.push_back(lit.second);
                }
                programLength = LOCCTR;
                break;
            }
        }
    }

    // After the main loop, before closing the intermediate file:
    if (intermediateFile.is_open()) {
        // Literals already written at END/LTORG. Just close.
        intermediateFile.close();
    } else {
         cerr << "Error: intermediate file stream was closed unexpectedly\n";
         return 1;
     }

    cout << "\nIntermediate file written to: " << intFilename << endl;

    // Display intermediate file on screen
    displayIntermediateFile(intFilename);

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
    
    return 0;
}

