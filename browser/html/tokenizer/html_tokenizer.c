#include "html_tokenizer.h"
#include "browser/html/diagnostics/html_diagnostics.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/string.h"

static int strncasecmp_custom(const char* s1, const char* s2, size_t n) {
    if (!s1 || !s2 || n == 0) return 0;
    while (n > 0 && *s1 && *s2) {
        char c1 = *s1;
        char c2 = *s2;
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        if (c1 != c2) return (c1 - c2);
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return (*s1 - *s2);
}

static void to_lowercase(char* str) {
    if (!str) return;
    while (*str) {
        if (*str >= 'A' && *str <= 'Z') {
            *str += 32;
        }
        str++;
    }
}

void html_tokenizer_init(html_tokenizer_t* tok, const char* html_input) {
    display_print("[TRACE] html_tokenizer_init: ENTER tok=0x"); display_print_hex((uint64_t)tok); display_print("\n");
    if (!tok) return;
    tok->input = html_input ? html_input : "";
    tok->length = strlen(tok->input);
    tok->position = 0;
    tok->line = 1;
    tok->column = 1;
    display_print("[TRACE] html_tokenizer_init: EXIT len="); display_print_dec(tok->length); display_print("\n");
}

void html_token_clear(bos_html_token_t* token) {
    display_print("[TRACE] html_token_clear: ENTER token=0x"); display_print_hex((uint64_t)token); display_print("\n");
    if (!token) return;
    bos_attr_t* curr = token->attributes;
    display_print("[TRACE] html_token_clear: attributes head=0x"); display_print_hex((uint64_t)curr); display_print("\n");
    while (curr) {
        uint64_t addr = (uint64_t)curr;
        // Verify pointer is in heap range [0x80000000, 0x84000000) before kfree
        if (addr >= 0x80000000ULL && addr < 0x84000000ULL) {
            bos_attr_t* next = curr->next;
            display_print("[TRACE] html_token_clear: calling kfree(0x"); display_print_hex(addr); display_print(")\n");
            kfree(curr);
            display_print("[TRACE] html_token_clear: kfree done, next=0x"); display_print_hex((uint64_t)next); display_print("\n");
            curr = next;
        } else {
            display_print("[TRACE] html_token_clear: attributes pointer 0x"); display_print_hex(addr); display_print(" is uninitialized/not in heap range, skipping kfree\n");
            break;
        }
    }
    display_print("[TRACE] html_token_clear: memsetting token\n");
    memset(token, 0, sizeof(bos_html_token_t));
    display_print("[TRACE] html_token_clear: EXIT\n");
}

static char peek_char(html_tokenizer_t* tok) {
    if (tok->position >= tok->length) return '\0';
    return tok->input[tok->position];
}

static char get_char(html_tokenizer_t* tok) {
    if (tok->position >= tok->length) return '\0';
    char c = tok->input[tok->position++];
    if (c == '\n') {
        tok->line++;
        tok->column = 1;
    } else {
        tok->column++;
    }
    return c;
}

static void decode_entities(char* text, size_t max_len) {
    display_print("[TRACE] decode_entities: ENTER text=0x"); display_print_hex((uint64_t)text); display_print("\n");
    if (!text || max_len == 0) return;
    static char buffer[512];
    size_t i = 0, j = 0;
    size_t len = strlen(text);

    while (i < len && j < sizeof(buffer) - 1 && j < max_len - 1) {
        if (text[i] == '&') {
            if (strncmp(&text[i], "&amp;", 5) == 0) {
                buffer[j++] = '&';
                i += 5;
            } else if (strncmp(&text[i], "&lt;", 4) == 0) {
                buffer[j++] = '<';
                i += 4;
            } else if (strncmp(&text[i], "&gt;", 4) == 0) {
                buffer[j++] = '>';
                i += 4;
            } else if (strncmp(&text[i], "&quot;", 6) == 0) {
                buffer[j++] = '"';
                i += 6;
            } else if (strncmp(&text[i], "&apos;", 6) == 0) {
                buffer[j++] = '\'';
                i += 6;
            } else if (strncmp(&text[i], "&nbsp;", 6) == 0) {
                buffer[j++] = ' ';
                i += 6;
            } else {
                buffer[j++] = text[i++];
            }
        } else {
            buffer[j++] = text[i++];
        }
    }
    buffer[j] = '\0';

    // Copy string without zero-padding rest of buffer (which strncpy does and corrupts stack)
    size_t k = 0;
    while (k < max_len - 1 && buffer[k] != '\0') {
        text[k] = buffer[k];
        k++;
    }
    text[k] = '\0';
    display_print("[TRACE] decode_entities: EXIT text="); display_print(text); display_print("\n");
}

bos_html_status_t html_tokenizer_next(html_tokenizer_t* tok, bos_html_token_t* out_token) {
    display_print("[TRACE] html_tokenizer_next: ENTER tok=0x"); display_print_hex((uint64_t)tok);
    display_print(" out_token=0x"); display_print_hex((uint64_t)out_token);
    display_print(" pos="); display_print_dec(tok ? tok->position : 0); display_print("\n");

    if (!tok || !out_token) return BOS_HTML_ERR_INVALID_PARAM;

    display_print("[TRACE] html_tokenizer_next: calling html_token_clear\n");
    html_token_clear(out_token);
    display_print("[TRACE] html_tokenizer_next: html_token_clear returned\n");

    if (tok->position >= tok->length) {
        out_token->type = BOS_TOKEN_EOF;
        display_print("[TRACE] html_tokenizer_next: EOF reached\n");
        return BOS_HTML_OK;
    }

    char c = peek_char(tok);

    // Tag or Comment or DOCTYPE
    if (c == '<') {
        get_char(tok); // consume '<'
        c = peek_char(tok);

        // DOCTYPE or Comment
        if (c == '!') {
            get_char(tok); // consume '!'
            if (strncmp(&tok->input[tok->position], "--", 2) == 0) {
                // Comment
                tok->position += 2;
                out_token->type = BOS_TOKEN_COMMENT;
                size_t idx = 0;
                while (tok->position < tok->length) {
                    if (strncmp(&tok->input[tok->position], "-->", 3) == 0) {
                        tok->position += 3;
                        break;
                    }
                    if (idx < sizeof(out_token->data) - 1) {
                        out_token->data[idx++] = get_char(tok);
                    } else {
                        get_char(tok);
                    }
                }
                out_token->data[idx] = '\0';
                html_diag_on_token();
                display_print("[TRACE] html_tokenizer_next: COMMENT parsed\n");
                return BOS_HTML_OK;
            } else if (strncasecmp_custom(&tok->input[tok->position], "DOCTYPE", 7) == 0) {
                // DOCTYPE
                tok->position += 7;
                out_token->type = BOS_TOKEN_DOCTYPE;
                // Skip spaces
                while (peek_char(tok) == ' ' || peek_char(tok) == '\t' || peek_char(tok) == '\n') get_char(tok);
                size_t idx = 0;
                while (tok->position < tok->length && peek_char(tok) != '>') {
                    if (idx < sizeof(out_token->name) - 1) {
                        out_token->name[idx++] = get_char(tok);
                    } else {
                        get_char(tok);
                    }
                }
                out_token->name[idx] = '\0';
                if (peek_char(tok) == '>') get_char(tok);
                to_lowercase(out_token->name);
                html_diag_on_token();
                display_print("[TRACE] html_tokenizer_next: DOCTYPE parsed\n");
                return BOS_HTML_OK;
            }
        }

        // End Tag </tag>
        if (c == '/') {
            get_char(tok); // consume '/'
            out_token->type = BOS_TOKEN_END_TAG;
            size_t idx = 0;
            while (tok->position < tok->length && peek_char(tok) != '>' && peek_char(tok) != ' ' && peek_char(tok) != '\t' && peek_char(tok) != '\n') {
                if (idx < sizeof(out_token->name) - 1) {
                    out_token->name[idx++] = get_char(tok);
                } else {
                    get_char(tok);
                }
            }
            out_token->name[idx] = '\0';
            to_lowercase(out_token->name);
            while (tok->position < tok->length && get_char(tok) != '>');
            html_diag_on_token();
            display_print("[TRACE] html_tokenizer_next: END_TAG parsed="); display_print(out_token->name); display_print("\n");
            return BOS_HTML_OK;
        }

        // Start Tag <tag attr="val">
        out_token->type = BOS_TOKEN_START_TAG;
        size_t idx = 0;
        while (tok->position < tok->length && peek_char(tok) != '>' && peek_char(tok) != '/' && peek_char(tok) != ' ' && peek_char(tok) != '\t' && peek_char(tok) != '\n') {
            if (idx < sizeof(out_token->name) - 1) {
                out_token->name[idx++] = get_char(tok);
            } else {
                get_char(tok);
            }
        }
        out_token->name[idx] = '\0';
        to_lowercase(out_token->name);
        display_print("[TRACE] html_tokenizer_next: START_TAG name="); display_print(out_token->name); display_print("\n");

        // Parse Attributes
        while (tok->position < tok->length) {
            c = peek_char(tok);
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                get_char(tok);
                continue;
            }
            if (c == '/' || c == '>') break;

            // Read attribute name
            char attr_name[64];
            size_t n_idx = 0;
            while (tok->position < tok->length && peek_char(tok) != '=' && peek_char(tok) != ' ' && peek_char(tok) != '\t' && peek_char(tok) != '\n' && peek_char(tok) != '>' && peek_char(tok) != '/') {
                if (n_idx < sizeof(attr_name) - 1) {
                    attr_name[n_idx++] = get_char(tok);
                } else {
                    get_char(tok);
                }
            }
            attr_name[n_idx] = '\0';
            to_lowercase(attr_name);

            char attr_val[256];
            attr_val[0] = '\0';

            // Skip spaces before '='
            while (peek_char(tok) == ' ' || peek_char(tok) == '\t' || peek_char(tok) == '\n') get_char(tok);

            if (peek_char(tok) == '=') {
                get_char(tok); // consume '='
                while (peek_char(tok) == ' ' || peek_char(tok) == '\t' || peek_char(tok) == '\n') get_char(tok);

                char quote = peek_char(tok);
                if (quote == '"' || quote == '\'') {
                    get_char(tok); // consume quote
                    size_t v_idx = 0;
                    while (tok->position < tok->length && peek_char(tok) != quote) {
                        if (v_idx < sizeof(attr_val) - 1) {
                            attr_val[v_idx++] = get_char(tok);
                        } else {
                            get_char(tok);
                        }
                    }
                    attr_val[v_idx] = '\0';
                    if (peek_char(tok) == quote) get_char(tok); // consume closing quote
                } else {
                    // Unquoted attribute value
                    size_t v_idx = 0;
                    while (tok->position < tok->length && peek_char(tok) != ' ' && peek_char(tok) != '\t' && peek_char(tok) != '\n' && peek_char(tok) != '>') {
                        if (v_idx < sizeof(attr_val) - 1) {
                            attr_val[v_idx++] = get_char(tok);
                        } else {
                            get_char(tok);
                        }
                    }
                    attr_val[v_idx] = '\0';
                }
            }

            decode_entities(attr_val, sizeof(attr_val));

            display_print("[TRACE] html_tokenizer_next: allocating bos_attr_t for "); display_print(attr_name); display_print("\n");
            bos_attr_t* attr = (bos_attr_t*)kmalloc(sizeof(bos_attr_t));
            display_print("[TRACE] html_tokenizer_next: kmalloc attr=0x"); display_print_hex((uint64_t)attr); display_print("\n");
            if (attr) {
                memset(attr, 0, sizeof(bos_attr_t));
                strncpy(attr->name, attr_name, sizeof(attr->name) - 1);
                strncpy(attr->value, attr_val, sizeof(attr->value) - 1);
                attr->next = out_token->attributes;
                out_token->attributes = attr;
                display_print("[TRACE] html_tokenizer_next: attached attr "); display_print(attr->name);
                display_print("="); display_print(attr->value); display_print(" at 0x"); display_print_hex((uint64_t)attr); display_print("\n");
            }
        }

        // Check self-closing '/'
        if (peek_char(tok) == '/') {
            out_token->self_closing = true;
            get_char(tok);
        }
        if (peek_char(tok) == '>') {
            get_char(tok);
        }

        html_diag_on_token();
        display_print("[TRACE] html_tokenizer_next: START_TAG complete\n");
        return BOS_HTML_OK;
    }

    // Text Token
    out_token->type = BOS_TOKEN_TEXT;
    size_t idx = 0;
    while (tok->position < tok->length && peek_char(tok) != '<') {
        if (idx < sizeof(out_token->data) - 1) {
            out_token->data[idx++] = get_char(tok);
        } else {
            get_char(tok);
        }
    }
    out_token->data[idx] = '\0';
    decode_entities(out_token->data, sizeof(out_token->data));

    html_diag_on_token();
    display_print("[TRACE] html_tokenizer_next: TEXT parsed data="); display_print(out_token->data); display_print("\n");
    return BOS_HTML_OK;
}

