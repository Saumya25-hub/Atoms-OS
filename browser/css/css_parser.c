#include "css_parser.h"
#include "css_tokenizer.h"
#include "css_selector.h"
#include "css_rule.h"
#include "css_stylesheet.h"
#include "kernel/core/lib/include/string.h"

css_status_t css_parser_parse_string(const char* css_input, css_stylesheet_t** out_sheet) {
    if (!out_sheet) return CSS_ERR_INVALID_PARAM;
    *out_sheet = NULL;

    css_stylesheet_t* sheet = NULL;
    if (css_stylesheet_create("inline", &sheet) != CSS_OK) {
        return CSS_ERR_OUT_OF_MEMORY;
    }

    css_tokenizer_t tok;
    css_tokenizer_init(&tok, css_input ? css_input : "");

    css_token_t token;

    while (1) {
        if (css_tokenizer_next(&tok, &token) != CSS_OK || token.type == CSS_TOKEN_EOF) {
            break;
        }

        // Read selector string until '{'
        char selector_buf[256];
        size_t s_idx = 0;

        while (token.type != CSS_TOKEN_EOF && !(token.type == CSS_TOKEN_SYMBOL && token.symbol == '{')) {
            bool need_space = false;
            if (s_idx > 0 && s_idx < sizeof(selector_buf) - 1) {
                char prev = selector_buf[s_idx - 1];
                if (prev != '#' && prev != '.' && prev != '>' && prev != '+' && prev != '~' &&
                    token.value[0] != '#' && token.value[0] != '.' && token.value[0] != '>' &&
                    token.value[0] != '+' && token.value[0] != '~') {
                    need_space = true;
                }
            }
            if (need_space) {
                selector_buf[s_idx++] = ' ';
            }

            if (token.type == CSS_TOKEN_HASH) {
                if (s_idx < sizeof(selector_buf) - 1) selector_buf[s_idx++] = '#';
            }

            size_t val_len = strlen(token.value);
            for (size_t i = 0; i < val_len && s_idx < sizeof(selector_buf) - 1; i++) {
                selector_buf[s_idx++] = token.value[i];
            }
            if (css_tokenizer_next(&tok, &token) != CSS_OK) break;
        }

        selector_buf[s_idx] = '\0';

        if (token.type == CSS_TOKEN_EOF) break;

        // Create Rule
        css_rule_t* rule = NULL;
        css_rule_create(CSS_RULE_STYLE, &rule);

        if (strlen(selector_buf) > 0) {
            css_selector_parse_string(selector_buf, &rule->selectors);
        }



        // Parse declarations inside '{ ... }'
        css_declaration_t* decl_tail = NULL;

        while (1) {
            if (css_tokenizer_next(&tok, &token) != CSS_OK || token.type == CSS_TOKEN_EOF) break;
            if (token.type == CSS_TOKEN_SYMBOL && token.symbol == '}') break;

            // Expect property name
            if (token.type != CSS_TOKEN_IDENT) {
                // Error recovery: skip to next ';' or '}'
                while (token.type != CSS_TOKEN_EOF && !(token.type == CSS_TOKEN_SYMBOL && (token.symbol == ';' || token.symbol == '}'))) {
                    css_tokenizer_next(&tok, &token);
                }
                if (token.type == CSS_TOKEN_SYMBOL && token.symbol == '}') break;
                continue;
            }

            char prop_name[64];
            size_t p_len = strlen(token.value);
            if (p_len >= sizeof(prop_name)) p_len = sizeof(prop_name) - 1;
            strncpy(prop_name, token.value, p_len);
            prop_name[p_len] = '\0';

            // Expect ':'
            if (css_tokenizer_next(&tok, &token) != CSS_OK || !(token.type == CSS_TOKEN_SYMBOL && token.symbol == ':')) {
                // Recovery: skip to ';' or '}'
                while (token.type != CSS_TOKEN_EOF && !(token.type == CSS_TOKEN_SYMBOL && (token.symbol == ';' || token.symbol == '}'))) {
                    css_tokenizer_next(&tok, &token);
                }
                if (token.type == CSS_TOKEN_SYMBOL && token.symbol == '}') break;
                continue;
            }

            // Read property value until ';' or '}'
            char val_buf[128];
            size_t v_idx = 0;

            while (1) {
                if (css_tokenizer_next(&tok, &token) != CSS_OK || token.type == CSS_TOKEN_EOF) break;
                if (token.type == CSS_TOKEN_SYMBOL && (token.symbol == ';' || token.symbol == '}')) break;

                bool need_space = false;
                if (v_idx > 0 && v_idx < sizeof(val_buf) - 1) {
                    char prev = val_buf[v_idx - 1];
                    if (prev != '(' && prev != ',' && token.value[0] != ')' && token.value[0] != ',') {
                        need_space = true;
                    }
                }
                if (need_space) {
                    val_buf[v_idx++] = ' ';
                }
                size_t tk_len = strlen(token.value);
                for (size_t i = 0; i < tk_len && v_idx < sizeof(val_buf) - 1; i++) {
                    val_buf[v_idx++] = token.value[i];
                }
            }

            val_buf[v_idx] = '\0';

            // Add declaration to rule
            if (strlen(prop_name) > 0 && strlen(val_buf) > 0) {
                css_declaration_t* decl = NULL;
                if (css_declaration_create(prop_name, val_buf, &decl) == CSS_OK) {
                    if (!decl_tail) {
                        rule->declarations = decl;
                        decl_tail = decl;
                    } else {
                        decl_tail->next = decl;
                        decl_tail = decl;
                    }
                }
            }

            if (token.type == CSS_TOKEN_SYMBOL && token.symbol == '}') break;
        }

        css_stylesheet_add_rule(sheet, rule);
    }

    *out_sheet = sheet;
    return CSS_OK;
}
