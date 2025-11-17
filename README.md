# SIC/XE Assembler – Pass 1 and Pass 2

## Team Members
- Bornhoft
- Jensen
- Johnson


## Build

Using Make (recommended):
- Clean and build both passes:
  ```
  make clean
  make
  ```

Manual build (no Makefile):
- Pass 1:
  ```
  g++ -std=c++11 -Wall -Wextra -g \
    Pass1.cpp SymbolTable.cpp LiteralTable.cpp OpcodeTable.cpp -o Pass1
  ```
- Pass 2:
  ```
  g++ -std=c++11 -Wall -Wextra -g \
    Pass2.cpp SymbolTable.cpp LiteralTable.cpp OpcodeTable.cpp -o Pass2
  ```

## Run

Pass 1 only (assemble to intermediate):
- Input: source .asm
- Output: .int (intermediate), Symbol Table (screen), Literal Table (screen)
  ```
  ./Pass1 test.asm
  ```

Pass 2 only (generate listing/object from intermediate):
- Input: .int
- Output: .txt (listing), .obj (object program; H/T/E records), Symbol & Literal tables appended to listing
  ```
  ./Pass2 test.int
  ```

End-to-end (Pass 1 then Pass 2):
```
./Pass1 test.asm && ./Pass2 test.int
```

Notes:
- Pass 2 accepts the .int produced by Pass 1 (same base name).
- Listing file is written to <base>.txt and object program to <base>.obj.
- Errors found during Pass 2 are summarized on screen.
