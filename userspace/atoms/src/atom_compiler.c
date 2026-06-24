#include "../include/atom_compiler.h"
#include "../include/atom_lexer.h"
#include "../include/atom_string.h"

static void print_dec(uint32_t num) {
    if (num == 0) { bos_print("0"); return; }
    char buf[16]; int i = 14; buf[15] = '\0';
    while (num > 0) { buf[i--] = (num % 10) + '0'; num /= 10; }
    bos_print(&buf[i+1]);
}

typedef struct {
    Token current;
    Token previous;
    bool had_error;
    bool panic_mode;
    AtomChunk* compiling_chunk;
} Parser;

static Parser parser;

static void error_at(Token* token, const char* message) {
    if (parser.panic_mode) return;
    parser.panic_mode = true;
    
    bos_print("[line ");
    print_dec(token->line);
    bos_print("] Error");
    
    if (token->type == TOKEN_EOF) {
        bos_print(" at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing
    } else {
        bos_print(" at '");
        for (int i = 0; i < token->length; i++) {
            // Note: simple print since bos_print doesn't take length.
            char c[2] = {token->start[i], '\0'};
            bos_print(c);
        }
        bos_print("'");
    }
    bos_print(": ");
    bos_print(message);
    bos_print("\n");
    parser.had_error = true;
}

static void error(const char* message) {
    error_at(&parser.previous, message);
}

static void error_at_current(const char* message) {
    error_at(&parser.current, message);
}

static void advance(void) {
    parser.previous = parser.current;
    
    for (;;) {
        parser.current = atom_lexer_scan_token();
        if (parser.current.type != TOKEN_ERROR) break;
        
        error_at_current(parser.current.start);
    }
}

static void consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }
    error_at_current(message);
}

static bool match(TokenType type) {
    if (parser.current.type != type) return false;
    advance();
    return true;
}

static void emit_byte(uint8_t byte) {
    atom_chunk_write(parser.compiling_chunk, byte);
}

static void emit_bytes(uint8_t byte1, uint8_t byte2) {
    emit_byte(byte1);
    emit_byte(byte2);
}

static void emit_return(void) {
    emit_byte(OP_RETURN);
}

static uint8_t make_constant(AtomValue value) {
    int constant = atom_chunk_add_constant(parser.compiling_chunk, value);
    if (constant > 255) {
        error("Too many constants in one chunk.");
        return 0;
    }
    return (uint8_t)constant;
}

static void emit_constant(AtomValue value) {
    emit_bytes(OP_CONSTANT, make_constant(value));
}

// Forward declarations
static void expression(void);

static void number(void) {
    double value = 0;
    for (int i = 0; i < parser.previous.length; i++) {
        value = value * 10 + (parser.previous.start[i] - '0');
    }
    emit_constant(atom_value_number(value));
}

static uint8_t identifier_constant(Token* name) {
    char buffer[256];
    int len = name->length;
    if (len > 255) len = 255;
    for (int i = 0; i < len; i++) buffer[i] = name->start[i];
    buffer[len] = '\0';
    
    AtomValue str_val = atom_string_create(buffer);
    return make_constant(str_val);
}

static void factor(void) {
    if (match(TOKEN_NUMBER)) {
        number();
    } else if (match(TOKEN_IDENTIFIER)) {
        uint8_t arg = identifier_constant(&parser.previous);
        if (match(TOKEN_EQUAL)) {
            expression();
            emit_bytes(OP_DEFINE_GLOBAL, arg);
        } else {
            emit_bytes(OP_GET_GLOBAL, arg);
        }
    } else {
        error("Expect expression.");
    }
}

static void term(void) {
    factor();
    while (match(TOKEN_PLUS)) {
        factor();
        emit_byte(OP_ADD);
    }
}

static void expression(void) {
    term();
}

static void statement(void) {
    if (match(TOKEN_PRINT)) {
        consume(TOKEN_LPAREN, "Expect '(' after print.");
        expression();
        consume(TOKEN_RPAREN, "Expect ')' after print arguments.");
        emit_byte(OP_PRINT);
    } else {
        expression();
    }
}

static void declaration(void) {
    statement();
    if (parser.panic_mode) {
        parser.panic_mode = false;
        // Simple sync
    }
}

bool atom_compiler_compile(const char* source, AtomChunk* chunk) {
    atom_lexer_init(source);
    parser.had_error = false;
    parser.panic_mode = false;
    parser.compiling_chunk = chunk;
    
    advance();
    
    while (!match(TOKEN_EOF)) {
        declaration();
    }
    
    emit_return();
    
    return !parser.had_error;
}
