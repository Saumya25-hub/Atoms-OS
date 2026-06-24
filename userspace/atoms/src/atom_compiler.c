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

static int emit_jump(uint8_t instruction) {
    emit_byte(instruction);
    emit_byte(0xff);
    emit_byte(0xff);
    return parser.compiling_chunk->count - 2;
}

static void patch_jump(int offset) {
    int jump = parser.compiling_chunk->count - offset - 2;
    if (jump > 65535) {
        error("Too much code to jump over.");
    }
    parser.compiling_chunk->code[offset] = (jump >> 8) & 0xff;
    parser.compiling_chunk->code[offset + 1] = jump & 0xff;
}

static void emit_loop(int loop_start) {
    emit_byte(OP_LOOP);
    int offset = parser.compiling_chunk->count - loop_start + 2;
    if (offset > 65535) error("Loop body too large.");
    emit_byte((offset >> 8) & 0xff);
    emit_byte(offset & 0xff);
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

static void string(void) {
    char buffer[512];
    int len = parser.previous.length - 2;
    if (len > 511) len = 511;
    for (int i = 0; i < len; i++) {
        buffer[i] = parser.previous.start[i + 1];
    }
    buffer[len] = '\0';
    
    AtomValue str_val = atom_string_create(buffer);
    emit_constant(str_val);
}

static void primary(void) {
    if (match(TOKEN_NUMBER)) {
        number();
    } else if (match(TOKEN_STRING)) {
        string();
    } else if (match(TOKEN_IDENTIFIER)) {
        uint8_t arg = identifier_constant(&parser.previous);
        
        if (match(TOKEN_LPAREN)) {
            emit_bytes(OP_GET_GLOBAL, arg);
            uint8_t arg_count = 0;
            if (parser.current.type != TOKEN_RPAREN) {
                do {
                    expression();
                    arg_count++;
                } while (match(TOKEN_COMMA));
            }
            consume(TOKEN_RPAREN, "Expect ')' after arguments.");
            emit_bytes(OP_CALL, arg_count);
        } else if (match(TOKEN_EQUAL)) {
            expression();
            emit_bytes(OP_DEFINE_GLOBAL, arg);
        } else {
            emit_bytes(OP_GET_GLOBAL, arg);
        }
    } else if (match(TOKEN_LPAREN)) {
        expression();
        consume(TOKEN_RPAREN, "Expect ')' after expression.");
    } else {
        error("Expect expression.");
    }
}

static void unary(void) {
    if (match(TOKEN_NOT)) {
        unary();
        emit_byte(OP_NOT);
    } else {
        primary();
    }
}

static void term(void) {
    unary();
    for (;;) {
        if (match(TOKEN_PLUS)) {
            unary();
            emit_byte(OP_ADD);
        } else if (match(TOKEN_MINUS)) {
            unary();
            emit_byte(OP_SUBTRACT);
        } else {
            break;
        }
    }
}

static void comparison(void) {
    term();
    for (;;) {
        if (match(TOKEN_GREATER)) {
            term();
            emit_byte(OP_GREATER);
        } else if (match(TOKEN_LESS)) {
            term();
            emit_byte(OP_LESS);
        } else if (match(TOKEN_GREATER_EQUAL)) {
            term();
            emit_bytes(OP_LESS, OP_NOT);
        } else if (match(TOKEN_LESS_EQUAL)) {
            term();
            emit_bytes(OP_GREATER, OP_NOT);
        } else {
            break;
        }
    }
}

static void equality(void) {
    comparison();
    for (;;) {
        if (match(TOKEN_EQUAL_EQUAL)) {
            comparison();
            emit_byte(OP_EQUAL);
        } else if (match(TOKEN_BANG_EQUAL)) {
            comparison();
            emit_bytes(OP_EQUAL, OP_NOT);
        } else {
            break;
        }
    }
}

static void logic_and(void) {
    equality();
    while (match(TOKEN_AND)) {
        equality();
        emit_byte(OP_AND);
    }
}

static void logic_or(void) {
    logic_and();
    while (match(TOKEN_OR)) {
        logic_and();
        emit_byte(OP_OR);
    }
}

static void expression(void) {
    logic_or();
}

static void statement(void);
static void declaration(void);

static void block(void) {
    while (!match(TOKEN_RBRACE) && !match(TOKEN_EOF)) {
        declaration();
    }
}

static void if_statement(void) {
    expression(); // Condition
    
    int then_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP); // Pop the condition
    
    consume(TOKEN_LBRACE, "Expect '{' before if body.");
    block();
    
    int else_jump = emit_jump(OP_JUMP);
    
    patch_jump(then_jump);
    emit_byte(OP_POP); // Pop the condition for false branch
    
    if (match(TOKEN_ELSE)) {
        consume(TOKEN_LBRACE, "Expect '{' before else body.");
        block();
    }
    
    patch_jump(else_jump);
}

static void while_statement(void) {
    int loop_start = parser.compiling_chunk->count;
    expression();
    
    int exit_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);
    
    consume(TOKEN_LBRACE, "Expect '{' before while body.");
    block();
    
    emit_loop(loop_start);
    
    patch_jump(exit_jump);
    emit_byte(OP_POP);
}

static void return_statement(void) {
    if (parser.current.type == TOKEN_RBRACE || parser.current.type == TOKEN_EOF) {
        emit_byte(OP_NIL);
    } else {
        expression();
    }
    emit_byte(OP_RETURN);
}

static void statement(void) {
    if (match(TOKEN_PRINT)) {
        consume(TOKEN_LPAREN, "Expect '(' after print.");
        expression();
        consume(TOKEN_RPAREN, "Expect ')' after print arguments.");
        emit_byte(OP_PRINT);
    } else if (match(TOKEN_IF)) {
        if_statement();
    } else if (match(TOKEN_WHILE)) {
        while_statement();
    } else if (match(TOKEN_RETURN)) {
        return_statement();
    } else if (match(TOKEN_LBRACE)) {
        block();
    } else {
        expression();
    }
}

#include "../include/atom_function.h"
static void function_declaration(void) {
    consume(TOKEN_IDENTIFIER, "Expect function name.");
    uint8_t global_name_arg = identifier_constant(&parser.previous);
    
    char name_buf[256];
    int len = parser.previous.length; if (len > 255) len = 255;
    for (int i=0; i<len; i++) name_buf[i] = parser.previous.start[i];
    name_buf[len] = '\0';
    
    AtomString* func_name = atom_string_create(name_buf).as.string;
    
    consume(TOKEN_LPAREN, "Expect '(' after function name.");
    
    int arity = 0;
    char param_names[8][256];
    if (parser.current.type != TOKEN_RPAREN) {
        do {
            consume(TOKEN_IDENTIFIER, "Expect parameter name.");
            len = parser.previous.length; if (len > 255) len = 255;
            for (int i=0; i<len; i++) param_names[arity][i] = parser.previous.start[i];
            param_names[arity][len] = '\0';
            arity++;
            if (arity >= 8) error("Cannot have more than 8 parameters.");
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RPAREN, "Expect ')' after parameters.");
    
    AtomFunction* function = atom_function_create(func_name, arity);
    for (int i=0; i<arity; i++) {
        function->param_names[i] = atom_string_create(param_names[i]).as.string;
    }
    
    consume(TOKEN_LBRACE, "Expect '{' before function body.");
    
    AtomChunk* previous_chunk = parser.compiling_chunk;
    parser.compiling_chunk = function->chunk;
    
    block();
    
    emit_byte(OP_NIL);
    emit_return();
    
    parser.compiling_chunk = previous_chunk;
    
    emit_constant(atom_value_function(function));
    emit_bytes(OP_DEFINE_GLOBAL, global_name_arg);
}

static void declaration(void) {
    if (match(TOKEN_FUNC)) {
        function_declaration();
    } else {
        statement();
    }
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
