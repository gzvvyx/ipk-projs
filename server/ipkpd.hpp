// Parser States
typedef enum ParserState {
    START,
    LPAR,
    EXP,
    RPAR,
    END
} ParserState;

// TCP States
typedef enum TCPState {
    INIT,
    ESTABLISHED
} TCPState;