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

static void CalculateRuleSpecificity(ABE_CSSRule* rule) {
    if (!rule) return;
    rule->specificity_a = 0;
    rule->specificity_b = 0;
    rule->specificity_c = 0;

    const char* p = rule->selector_str;
    while (*p != '\0') {
        if (*p == '#') {
            rule->specificity_a++; p++;
        } else if (*p == '.') {
            rule->specificity_b++; p++;
        } else if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z')) {
            // Count element tag name
            if (p == rule->selector_str || *(p - 1) == ' ' || *(p - 1) == '>' || *(p - 1) == '+' || *(p - 1) == '~') {
                rule->specificity_c++;
            }
            p++;
        } else {
            p++;
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
    while (ABE_CSSTokenizer_NextToken(&tok, &token) == ABE_SUCCESS) {
        if (token.type == CSSTOKEN_EOF) break;

        if (token.type == CSSTOKEN_IDENT || token.type == CSSTOKEN_HASH || token.type == CSSTOKEN_DELIM) {
            if (sheet->rule_count >= ABE_MAX_CSS_RULES) break;

            ABE_CSSRule* rule = &sheet->rules[sheet->rule_count++];
            rule->rule_index = sheet->rule_count;

            // 1. Accumulate Selector String until '{'
            size_t sel_len = 0;
            while (token.type != CSSTOKEN_LBRACE && token.type != CSSTOKEN_EOF) {
                if (token.type == CSSTOKEN_IDENT || token.type == CSSTOKEN_HASH || token.type == CSSTOKEN_STRING) {
                    if (sel_len > 0 && rule->selector_str[sel_len - 1] != '.' && rule->selector_str[sel_len - 1] != '#' && rule->selector_str[sel_len - 1] != ':') {
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

                    // Expect Value
                    ABE_CSSTokenizer_NextToken(&tok, &token);
                    if (token.type == CSSTOKEN_NUMBER) {
                        decl->value.type = CSS_VAL_PX;
                        decl->value.number_val = token.number_val;
                    } else if (token.type == CSSTOKEN_DIMENSION) {
                        if (StrCaseCmp(token.unit, "px") == 0) decl->value.type = CSS_VAL_PX;
                        else if (StrCaseCmp(token.unit, "em") == 0) decl->value.type = CSS_VAL_EM;
                        else if (StrCaseCmp(token.unit, "rem") == 0) decl->value.type = CSS_VAL_REM;
                        else decl->value.type = CSS_VAL_PX;
                        decl->value.number_val = token.number_val;
                    } else if (token.type == CSSTOKEN_PERCENTAGE) {
                        decl->value.type = CSS_VAL_PERCENT;
                        decl->value.number_val = token.number_val;
                    } else if (token.type == CSSTOKEN_HASH) {
                        decl->value.type = CSS_VAL_COLOR;
                        decl->value.color_val = ABE_CSSTokenizer_ParseColorHex(token.value);
                    } else if (token.type == CSSTOKEN_IDENT) {
                        if (StrCaseCmp(token.value, "auto") == 0) decl->value.type = CSS_VAL_AUTO;
                        else if (StrCaseCmp(token.value, "inherit") == 0) decl->value.type = CSS_VAL_INHERIT;
                        else if (StrCaseCmp(token.value, "initial") == 0) decl->value.type = CSS_VAL_INITIAL;
                        else if (StrCaseCmp(token.value, "red") == 0) { decl->value.type = CSS_VAL_COLOR; decl->value.color_val = 0xFFFF0000; }
                        else if (StrCaseCmp(token.value, "blue") == 0) { decl->value.type = CSS_VAL_COLOR; decl->value.color_val = 0xFF0000FF; }
                        else if (StrCaseCmp(token.value, "green") == 0) { decl->value.type = CSS_VAL_COLOR; decl->value.color_val = 0xFF008000; }
                        else if (StrCaseCmp(token.value, "black") == 0) { decl->value.type = CSS_VAL_COLOR; decl->value.color_val = 0xFF000000; }
                        else if (StrCaseCmp(token.value, "white") == 0) { decl->value.type = CSS_VAL_COLOR; decl->value.color_val = 0xFFFFFFFF; }
                        else {
                            decl->value.type = CSS_VAL_IDENT;
                            strncpy(decl->value.str_val, token.value, sizeof(decl->value.str_val) - 1);
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
