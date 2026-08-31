#include "abe_css_parser.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static ABE_CSSStylesheetManager g_sheet_mgr;
static uint32_t g_next_sheet_id = 7000;
static bool g_css_parser_initialized = false;

static char ToLowerChar(char c) {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

static int StrCaseCmp(const char* s1, const char* s2) {
    if (!s1 || !s2) return 1;
    while (*s1 && *s2) {
        char c1 = ToLowerChar(*s1);
        char c2 = ToLowerChar(*s2);
        if (c1 != c2) return 1;
        s1++; s2++;
    }
    return (*s1 == '\0' && *s2 == '\0') ? 0 : 1;
}

ABE_Error ABE_CSSParser_Init(void) {
    memset(&g_sheet_mgr, 0, sizeof(ABE_CSSStylesheetManager));
    g_css_parser_initialized = true;
    ABE_Log(ABE_LOG_INFO, "CSS", "ABE Production CSS Ruleset & Declaration Parser initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_CSSParser_Shutdown(void) {
    if (!g_css_parser_initialized) return ABE_ERR_NOT_INITIALIZED;
    for (uint32_t i = 0; i < ABE_MAX_STYLESHEETS; i++) {
        if (g_sheet_mgr.stylesheets[i].in_use) {
            ABE_CSSParser_DestroyStylesheet(g_sheet_mgr.stylesheets[i].handle);
        }
    }
    g_css_parser_initialized = false;
    ABE_Log(ABE_LOG_INFO, "CSS", "ABE CSS Parser shut down cleanly");
    return ABE_SUCCESS;
}

ABE_CSSStylesheet* ABE_CSSParser_GetStylesheet(ABE_StylesheetHandle handle) {
    if (!g_css_parser_initialized || handle == ABE_INVALID_HANDLE) return NULL;
    uint32_t slot = (handle >> 16) & 0xFFFF;
    if (slot >= ABE_MAX_STYLESHEETS) return NULL;
    if (g_sheet_mgr.stylesheets[slot].handle == handle && g_sheet_mgr.stylesheets[slot].in_use) {
        return &g_sheet_mgr.stylesheets[slot];
    }
    return NULL;
}

const char* ABE_CSSParser_GetCustomProperty(ABE_StylesheetHandle handle, const char* name) {
    if (!name) return NULL;
    ABE_CSSStylesheet* sheet = ABE_CSSParser_GetStylesheet(handle);
    if (!sheet) return NULL;

    for (uint32_t i = 0; i < sheet->custom_property_count; i++) {
        if (strcmp(sheet->custom_properties[i].name, name) == 0) {
            return sheet->custom_properties[i].value;
        }
    }
    return NULL;
}

static void CalculateRuleSpecificity(ABE_CSSRule* rule) {
    if (!rule) return;
    rule->specificity_a = 0;
    rule->specificity_b = 0;
    rule->specificity_c = 0;

    const char* p = rule->selector_str;
    while (*p != '\0') {
        if (*p == '#') {
            rule->specificity_a++;
            p++;
        } else if (*p == '.' || *p == '[' || (*p == ':' && *(p + 1) != ':')) {
            rule->specificity_b++;
            p++;
        } else if (*p == ':' && *(p + 1) == ':') {
            rule->specificity_c++;
            p += 2;
        } else if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z')) {
            // Count element tag name if at boundary
            if (p == rule->selector_str || *(p - 1) == ' ' || *(p - 1) == '>' || *(p - 1) == '+' || *(p - 1) == '~' || *(p - 1) == ',') {
                rule->specificity_c++;
            }
            p++;
        } else {
            p++;
        }
    }
}

static void ParseValueToken(ABE_CSSTokenizer* tok, ABE_CSSToken* token, ABE_CSSDeclaration* decl) {
    if (token->type == CSSTOKEN_NUMBER) {
        decl->value.type = CSS_VAL_PX;
        decl->value.number_val = token->number_val;
    } else if (token->type == CSSTOKEN_DIMENSION) {
        if (StrCaseCmp(token->unit, "px") == 0) decl->value.type = CSS_VAL_PX;
        else if (StrCaseCmp(token->unit, "em") == 0) decl->value.type = CSS_VAL_EM;
        else if (StrCaseCmp(token->unit, "rem") == 0) decl->value.type = CSS_VAL_REM;
        else if (StrCaseCmp(token->unit, "vw") == 0) decl->value.type = CSS_VAL_VW;
        else if (StrCaseCmp(token->unit, "vh") == 0) decl->value.type = CSS_VAL_VH;
        else if (StrCaseCmp(token->unit, "pt") == 0) { decl->value.type = CSS_VAL_PX; token->number_val *= 1.333f; }
        else decl->value.type = CSS_VAL_PX;
        decl->value.number_val = token->number_val;
    } else if (token->type == CSSTOKEN_PERCENTAGE) {
        decl->value.type = CSS_VAL_PERCENT;
        decl->value.number_val = token->number_val;
    } else if (token->type == CSSTOKEN_HASH) {
        decl->value.type = CSS_VAL_COLOR;
        decl->value.color_val = ABE_CSSTokenizer_ParseColorHex(token->value);
    } else if (token->type == CSSTOKEN_FUNCTION) {
        if (StrCaseCmp(token->value, "var") == 0) {
            decl->value.type = CSS_VAL_VAR;
            ABE_CSSToken arg_token;
            if (ABE_CSSTokenizer_NextToken(tok, &arg_token) == ABE_SUCCESS) {
                strncpy(decl->value.str_val, arg_token.value, sizeof(decl->value.str_val) - 1);
            }
            while (arg_token.type != CSSTOKEN_RPAREN && arg_token.type != CSSTOKEN_EOF) {
                if (ABE_CSSTokenizer_NextToken(tok, &arg_token) != ABE_SUCCESS) break;
            }
        } else if (StrCaseCmp(token->value, "rgb") == 0 || StrCaseCmp(token->value, "rgba") == 0) {
            char color_buf[64];
            strncpy(color_buf, token->value, sizeof(color_buf) - 1);
            strcat(color_buf, "(");
            ABE_CSSToken arg_tok;
            while (ABE_CSSTokenizer_NextToken(tok, &arg_tok) == ABE_SUCCESS) {
                if (arg_tok.type == CSSTOKEN_RPAREN || arg_tok.type == CSSTOKEN_EOF) break;
                if (arg_tok.type == CSSTOKEN_NUMBER) {
                    char num_s[16];
                    int n = (int)arg_tok.number_val;
                    char* p = num_s;
                    if (n == 0) *p++ = '0';
                    else {
                        char tmp[16]; int ti = 0;
                        while (n > 0) { tmp[ti++] = (n % 10) + '0'; n /= 10; }
                        while (ti > 0) *p++ = tmp[--ti];
                    }
                    *p = '\0';
                    strcat(color_buf, num_s);
                } else if (arg_tok.type == CSSTOKEN_COMMA) {
                    strcat(color_buf, ",");
                }
            }
            strcat(color_buf, ")");
            decl->value.type = CSS_VAL_COLOR;
            decl->value.color_val = ABE_CSSTokenizer_ParseColor(color_buf);
        } else {
            decl->value.type = CSS_VAL_IDENT;
            strncpy(decl->value.str_val, token->value, sizeof(decl->value.str_val) - 1);
        }
    } else if (token->type == CSSTOKEN_IDENT || token->type == CSSTOKEN_STRING) {
        if (StrCaseCmp(token->value, "auto") == 0) decl->value.type = CSS_VAL_AUTO;
        else if (StrCaseCmp(token->value, "inherit") == 0) decl->value.type = CSS_VAL_INHERIT;
        else if (StrCaseCmp(token->value, "initial") == 0) decl->value.type = CSS_VAL_INITIAL;
        else if (StrCaseCmp(token->value, "none") == 0) decl->value.type = CSS_VAL_NONE;
        else {
            uint32_t named_c = ABE_CSSTokenizer_ParseColor(token->value);
            if (named_c != 0xFF000000 || StrCaseCmp(token->value, "black") == 0) {
                decl->value.type = CSS_VAL_COLOR;
                decl->value.color_val = named_c;
            } else {
                decl->value.type = CSS_VAL_IDENT;
                strncpy(decl->value.str_val, token->value, sizeof(decl->value.str_val) - 1);
            }
        }
    }
}

ABE_Error ABE_CSSParser_ParseStylesheet(const char* css_str, size_t len, ABE_CSSOrigin origin, ABE_StylesheetHandle* out_sheet) {
    if (!g_css_parser_initialized || !css_str || len == 0 || !out_sheet) return ABE_ERR_INVALID_PARAM;
    if (g_sheet_mgr.active_count >= ABE_MAX_STYLESHEETS) return ABE_ERR_RESOURCE_EXHAUSTED;

    uint32_t slot = ABE_INVALID_HANDLE;
    for (uint32_t i = 0; i < ABE_MAX_STYLESHEETS; i++) {
        if (!g_sheet_mgr.stylesheets[i].in_use) {
            slot = i;
            break;
        }
    }

    if (slot == ABE_INVALID_HANDLE) return ABE_ERR_RESOURCE_EXHAUSTED;

    ABE_CSSStylesheet* sheet = &g_sheet_mgr.stylesheets[slot];
    memset(sheet, 0, sizeof(ABE_CSSStylesheet));
    sheet->handle = (g_next_sheet_id++) | (slot << 16);
    sheet->origin = origin;
    sheet->in_use = true;

    ABE_CSSTokenizer tok;
    ABE_CSSTokenizer_Init(&tok, css_str, len);

    ABE_CSSToken token;
    ABE_CSSMediaQuery cur_media;
    memset(&cur_media, 0, sizeof(cur_media));
    bool in_media_block = false;

    while (ABE_CSSTokenizer_NextToken(&tok, &token) == ABE_SUCCESS) {
        if (token.type == CSSTOKEN_EOF) break;

        // Check At-Rule (@media, @import)
        if (token.type == CSSTOKEN_AT_RULE) {
            if (StrCaseCmp(token.value, "media") == 0) {
                memset(&cur_media, 0, sizeof(cur_media));
                cur_media.has_media_query = true;

                // Parse media query condition until '{'
                while (token.type != CSSTOKEN_LBRACE && token.type != CSSTOKEN_EOF) {
                    if (token.type == CSSTOKEN_IDENT && StrCaseCmp(token.value, "max-width") == 0) {
                        ABE_CSSTokenizer_NextToken(&tok, &token); // ':'
                        ABE_CSSToken val_tok;
                        if (ABE_CSSTokenizer_NextToken(&tok, &val_tok) == ABE_SUCCESS) {
                            cur_media.max_width_px = val_tok.number_val;
                        }
                    } else if (token.type == CSSTOKEN_IDENT && StrCaseCmp(token.value, "min-width") == 0) {
                        ABE_CSSTokenizer_NextToken(&tok, &token); // ':'
                        ABE_CSSToken val_tok;
                        if (ABE_CSSTokenizer_NextToken(&tok, &val_tok) == ABE_SUCCESS) {
                            cur_media.min_width_px = val_tok.number_val;
                        }
                    }
                    ABE_CSSTokenizer_NextToken(&tok, &token);
                }
                in_media_block = true;
                continue;
            } else {
                // Skip unsupported at-rule until ';' or '}'
                while (token.type != CSSTOKEN_SEMICOLON && token.type != CSSTOKEN_RBRACE && token.type != CSSTOKEN_EOF) {
                    ABE_CSSTokenizer_NextToken(&tok, &token);
                }
                continue;
            }
        }

        if (in_media_block && token.type == CSSTOKEN_RBRACE) {
            in_media_block = false;
            memset(&cur_media, 0, sizeof(cur_media));
            continue;
        }

        // Custom Property Definition (--atoms-var: value;)
        if (token.type == CSSTOKEN_IDENT && strncmp(token.value, "--", 2) == 0) {
            if (sheet->custom_property_count < ABE_MAX_CUSTOM_PROPERTIES) {
                ABE_CSSCustomProperty* cp = &sheet->custom_properties[sheet->custom_property_count++];
                strncpy(cp->name, token.value, sizeof(cp->name) - 1);
                ABE_CSSTokenizer_NextToken(&tok, &token); // ':'
                ABE_CSSToken val_tok;
                if (ABE_CSSTokenizer_NextToken(&tok, &val_tok) == ABE_SUCCESS) {
                    strncpy(cp->value, val_tok.value, sizeof(cp->value) - 1);
                }
            }
            continue;
        }

        if (token.type == CSSTOKEN_IDENT || token.type == CSSTOKEN_HASH || token.type == CSSTOKEN_DELIM || token.type == CSSTOKEN_COLON || token.type == CSSTOKEN_LBRACKET) {
            if (sheet->rule_count >= ABE_MAX_CSS_RULES) break;

            ABE_CSSRule* rule = &sheet->rules[sheet->rule_count++];
            rule->rule_index = sheet->rule_count;
            rule->media_query = cur_media;

            // 1. Accumulate Selector String until '{'
            size_t sel_len = 0;
            while (token.type != CSSTOKEN_LBRACE && token.type != CSSTOKEN_EOF) {
                if (token.type == CSSTOKEN_IDENT || token.type == CSSTOKEN_HASH || token.type == CSSTOKEN_STRING) {
                    if (sel_len > 0 && rule->selector_str[sel_len - 1] != '.' && rule->selector_str[sel_len - 1] != '#' && rule->selector_str[sel_len - 1] != ':' && rule->selector_str[sel_len - 1] != '[' && rule->selector_str[sel_len - 1] != '>') {
                        if (sel_len < sizeof(rule->selector_str) - 1) rule->selector_str[sel_len++] = ' ';
                    }
                    if (token.type == CSSTOKEN_HASH) {
                        if (sel_len < sizeof(rule->selector_str) - 1) rule->selector_str[sel_len++] = '#';
                    }
                    size_t vlen = strlen(token.value);
                    if (sel_len + vlen < sizeof(rule->selector_str) - 1) {
                        strncpy(rule->selector_str + sel_len, token.value, sizeof(rule->selector_str) - 1 - sel_len);
                        sel_len += vlen;
                    }
                } else if (token.type == CSSTOKEN_DELIM) {
                    if (sel_len < sizeof(rule->selector_str) - 1) {
                        rule->selector_str[sel_len++] = token.delim;
                    }
                } else if (token.type == CSSTOKEN_COLON) {
                    if (sel_len < sizeof(rule->selector_str) - 1) rule->selector_str[sel_len++] = ':';
                } else if (token.type == CSSTOKEN_LBRACKET) {
                    if (sel_len < sizeof(rule->selector_str) - 1) rule->selector_str[sel_len++] = '[';
                } else if (token.type == CSSTOKEN_RBRACKET) {
                    if (sel_len < sizeof(rule->selector_str) - 1) rule->selector_str[sel_len++] = ']';
                } else if (token.type == CSSTOKEN_COMMA) {
                    if (sel_len < sizeof(rule->selector_str) - 1) rule->selector_str[sel_len++] = ',';
                }
                if (ABE_CSSTokenizer_NextToken(&tok, &token) != ABE_SUCCESS) break;
            }
            rule->selector_str[sel_len] = '\0';
            CalculateRuleSpecificity(rule);

            // 2. Parse Declaration Block until '}'
            while (token.type != CSSTOKEN_RBRACE && token.type != CSSTOKEN_EOF) {
                if (ABE_CSSTokenizer_NextToken(&tok, &token) != ABE_SUCCESS) break;
                if (token.type == CSSTOKEN_RBRACE || token.type == CSSTOKEN_EOF) break;

                if (token.type == CSSTOKEN_IDENT) {
                    if (rule->declaration_count >= ABE_MAX_CSS_DECLARATIONS) continue;
                    ABE_CSSDeclaration* decl = &rule->declarations[rule->declaration_count++];
                    strncpy(decl->name, token.value, sizeof(decl->name) - 1);

                    // Expect ':'
                    ABE_CSSTokenizer_NextToken(&tok, &token);
                    if (token.type != CSSTOKEN_COLON) {
                        ABE_Diag_RecordCSSErrorRecovered();
                        continue;
                    }

                    // Expect Value Token
                    ABE_CSSTokenizer_NextToken(&tok, &token);
                    ParseValueToken(&tok, &token, decl);

                    // Check for trailing `!important` or multiple values before ';' or '}'
                    ABE_CSSToken lookahead;
                    while (ABE_CSSTokenizer_NextToken(&tok, &lookahead) == ABE_SUCCESS) {
                        if (lookahead.type == CSSTOKEN_IMPORTANT) {
                            decl->is_important = true;
                        } else if (lookahead.type == CSSTOKEN_SEMICOLON || lookahead.type == CSSTOKEN_RBRACE || lookahead.type == CSSTOKEN_EOF) {
                            token = lookahead;
                            break;
                        }
                    }
                }
            }
        }
    }

    g_sheet_mgr.active_count++;
    *out_sheet = sheet->handle;
    ABE_Diag_RecordStylesheetParsed(sheet->rule_count);
    ABE_LogVal(ABE_LOG_INFO, "CSS", "Parsed Stylesheet cleanly, Rule count: ", sheet->rule_count);
    return ABE_SUCCESS;
}

ABE_Error ABE_CSSParser_DestroyStylesheet(ABE_StylesheetHandle handle) {
    if (!g_css_parser_initialized || handle == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;
    ABE_CSSStylesheet* sheet = ABE_CSSParser_GetStylesheet(handle);
    if (!sheet) return ABE_ERR_INVALID_PARAM;

    sheet->in_use = false;
    if (g_sheet_mgr.active_count > 0) g_sheet_mgr.active_count--;
    ABE_LogVal(ABE_LOG_INFO, "CSS", "Destroyed Stylesheet, Handle: ", handle);
    return ABE_SUCCESS;
}

