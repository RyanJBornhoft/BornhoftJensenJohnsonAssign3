COPY    START   1000
FIRST   STL     RETADR
        LDB     #LENGTH
        BASE    LENGTH
CLOOP   +JSUB   RDREC
        LDA     LENGTH
        COMP    #0
        JEQ     ENDFIL
        +JSUB   WRREC
        J       CLOOP
ENDFIL  LDA     =C'EOF'
        STA     BUFFER
        LDA     #3
        STA     LENGTH
        +JSUB   WRREC
        J       @RETADR
        LTORG
RETADR  RESW    1
LENGTH  RESW    1
BUFFER  RESB    4096
.
. SUBROUTINE TO READ RECORD INTO BUFFER
.
RDREC   CLEAR   X
        CLEAR   A
        CLEAR   S
        +LDT    #4096
RLOOP   TD      INPUT
        JEQ     RLOOP
        RD      INPUT
        STCH    BUFFER,X
        TIXR    T
        JLT     RLOOP
        STX     LENGTH
        RSUB
INPUT   BYTE    X'F1'
.
. SUBROUTINE TO WRITE RECORD FROM BUFFER
.
WRREC   CLEAR   X
        LDT     LENGTH
WLOOP   TD      =X'05'
        JEQ     WLOOP
        LDCH    BUFFER,X
        WD      OUTPUT
        TIXR    T
        JLT     WLOOP
        RSUB
OUTPUT  BYTE    X'06'
        END     FIRST