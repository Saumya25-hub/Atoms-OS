#include "abe_html_tokenizer.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static bool IsWhitespace(char c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f');
}

static char ToLower(char c) {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

void ABE_HTMLTokenizer_DecodeEntities(const char* in_str, char* out_str, size_t max_len) {
    if (!in_str || !out_str || max_len == 0) return;

    size_t in_i = 0;
    size_t out_i = 0;

    while (in_str[in_i] != '\0' && out_i < max_len - 1) {
        if (in_str[in_i] == '&') {
            if (strncmp(in_str + in_i, "&amp;", 5) == 0) {
                out_str[out_i++] = '&'; in_i += 5;
            } else if (strncmp(in_str + in_i, "&lt;", 4) == 0) {
                out_str[out_i++] = '<'; in_i += 4;
            } else if (strncmp(in_str + in_i, "&gt;", 4) == 0) {
                out_str[out_i++] = '>'; in_i += 4;
            } else if (strncmp(in_str + in_i, "&quot;", 6) == 0) {
                out_str[out_i++] = '"'; in_i += 6;
            } else if (strncmp(in_str + in_i, "&#39;", 5) == 0 || strncmp(in_str + in_i, "&apos;", 6) == 0) {
                out_str[out_i++] = '\''; in_i += (in_str[in_i + 1] == '#' ? 5 : 6);
            } else if (strncmp(in_str + in_i, "&nbsp;", 6) == 0) {
                out_str[out_i++] = ' '; in_i += 6;
            } else {
                out_str[out_i++] = in_str[in_i++];
            }
        } else {
            out_str[out_i++] = in_str[in_i++];
        }
    }
    out_str[out_i] = '\0';
}

ABE_Error ABE_HTMLTokenizer_Init(ABE_HTMLTokenizer* tok, const char* input, size_t len) {
    if (!tok || !input) return ABE_ERR_INVALID_PARAM;
    memset(tok, 0, sizeof(ABE_HTMLTokenizer));
    tok->input = input;
    tok->input_len = len;
    tok->position = 0;
    tok->state = STATE_DATA;
    return ABE_SUCCESS;
}

ABE_Error ABE_HTMLTokenizer_NextToken(ABE_HTMLTokenizer* tok, ABE_HTMLToken* out_token) {
    if (!tok || !tok->input || !out_token) return ABE_ERR_INVALID_PARAM;
    memset(out_token, 0, sizeof(ABE_HTMLToken));

    if (tok->position >= tok->input_len) {
        out_token->type = TOKEN_EOF;
        return ABE_SUCCESS;
    }

    size_t val_idx = 0;
    size_t tag_idx = 0;
    size_t attr_name_idx = 0;
    size_t attr_val_idx = 0;

    while (tok->position < tok->input_len) {
        char c = tok->input[tok->position];

        switch (tok->state) {
            case STATE_DATA:
                if (c == '<') {
                    if (val_idx > 0) {
                        out_token->type = TOKEN_CHARACTER;
                        out_token->value[val_idx] = '\0';
                        char decoded[512];
                        ABE_HTMLTokenizer_DecodeEntities(out_token->value, decoded, sizeof(decoded));
                        strncpy(out_token->value, decoded, sizeof(out_token->value) - 1);
                        ABE_Diag_RecordHTMLToken();
                        return ABE_SUCCESS;
                    }
                    tok->position++;
                    tok->state = STATE_TAG_OPEN;
                } else {
                    if (val_idx < sizeof(out_token->value) - 1) {
                        out_token->value[val_idx++] = c;
                    }
                    tok->position++;
                }
                break;

            case STATE_TAG_OPEN:
                if (c == '/') {
                    tok->position++;
                    tok->state = STATE_END_TAG_OPEN;
                } else if (c == '!') {
                    tok->position++;
                    tok->state = STATE_COMMENT;
                } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                    out_token->type = TOKEN_START_TAG;
                    tag_idx = 0;
                    tok->state = STATE_TAG_NAME;
                } else {
                    tok->state = STATE_DATA;
                }
                break;

            case STATE_END_TAG_OPEN:
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                    out_token->type = TOKEN_END_TAG;
                    tag_idx = 0;
                    tok->state = STATE_TAG_NAME;
                } else {
                    tok->position++;
                    tok->state = STATE_DATA;
                }
                break;

            case STATE_TAG_NAME:
                if (IsWhitespace(c)) {
                    out_token->tag_name[tag_idx] = '\0';
                    tok->position++;
                    tok->state = STATE_BEFORE_ATTR_NAME;
                } else if (c == '/') {
                    out_token->tag_name[tag_idx] = '\0';
                    tok->position++;
                    tok->state = STATE_SELF_CLOSING_TAG;
                } else if (c == '>') {
                    out_token->tag_name[tag_idx] = '\0';
                    tok->position++;
                    tok->state = STATE_DATA;
                    ABE_Diag_RecordHTMLToken();
                    return ABE_SUCCESS;
                } else {
                    if (tag_idx < sizeof(out_token->tag_name) - 1) {
                        out_token->tag_name[tag_idx++] = ToLower(c);
                    }
                    tok->position++;
                }
                break;

            case STATE_BEFORE_ATTR_NAME:
                if (IsWhitespace(c)) {
                    tok->position++;
                } else if (c == '>') {
                    tok->position++;
                    tok->state = STATE_DATA;
                    ABE_Diag_RecordHTMLToken();
                    return ABE_SUCCESS;
                } else if (c == '/') {
                    tok->position++;
                    tok->state = STATE_SELF_CLOSING_TAG;
                } else {
                    attr_name_idx = 0;
                    tok->current_attr_name[0] = '\0';
                    tok->state = STATE_ATTR_NAME;
                }
                break;

            case STATE_ATTR_NAME:
                if (c == '=' || IsWhitespace(c) || c == '>') {
                    tok->current_attr_name[attr_name_idx] = '\0';
                    if (c == '=') {
                        tok->position++;
                        tok->state = STATE_BEFORE_ATTR_VAL;
                    } else if (IsWhitespace(c)) {
                        tok->position++;
                        tok->state = STATE_AFTER_ATTR_NAME;
                    } else {
                        // Attribute without value
                        if (out_token->attribute_count < ABE_MAX_ATTRIBUTES) {
                            ABE_DOMAttributeInfo* a = &out_token->attributes[out_token->attribute_count++];
                            strncpy(a->name, tok->current_attr_name, sizeof(a->name) - 1);
                            a->value[0] = '\0';
                        }
                        tok->position++;
                        tok->state = STATE_DATA;
                        ABE_Diag_RecordHTMLToken();
                        return ABE_SUCCESS;
                    }
                } else {
                    if (attr_name_idx < sizeof(tok->current_attr_name) - 1) {
                        tok->current_attr_name[attr_name_idx++] = ToLower(c);
                    }
                    tok->position++;
                }
                break;

            case STATE_AFTER_ATTR_NAME:
                if (IsWhitespace(c)) {
                    tok->position++;
                } else if (c == '=') {
                    tok->position++;
                    tok->state = STATE_BEFORE_ATTR_VAL;
                } else {
                    if (out_token->attribute_count < ABE_MAX_ATTRIBUTES) {
                        ABE_DOMAttributeInfo* a = &out_token->attributes[out_token->attribute_count++];
                        strncpy(a->name, tok->current_attr_name, sizeof(a->name) - 1);
                        a->value[0] = '\0';
                    }
                    tok->state = STATE_BEFORE_ATTR_NAME;
                }
                break;

            case STATE_BEFORE_ATTR_VAL:
                if (IsWhitespace(c)) {
                    tok->position++;
                } else if (c == '"') {
                    tok->position++;
                    attr_val_idx = 0;
                    tok->state = STATE_ATTR_VAL_DOUBLE_QUOTED;
                } else if (c == '\'') {
                    tok->position++;
                    attr_val_idx = 0;
                    tok->state = STATE_ATTR_VAL_SINGLE_QUOTED;
                } else {
                    attr_val_idx = 0;
                    tok->state = STATE_ATTR_VAL_UNQUOTED;
                }
                break;

            case STATE_ATTR_VAL_DOUBLE_QUOTED:
                if (c == '"') {
                    tok->current_attr_val[attr_val_idx] = '\0';
                    if (out_token->attribute_count < ABE_MAX_ATTRIBUTES) {
                        ABE_DOMAttributeInfo* a = &out_token->attributes[out_token->attribute_count++];
                        strncpy(a->name, tok->current_attr_name, sizeof(a->name) - 1);
                        ABE_HTMLTokenizer_DecodeEntities(tok->current_attr_val, a->value, sizeof(a->value));
                    }
                    tok->position++;
                    tok->state = STATE_BEFORE_ATTR_NAME;
                } else {
                    if (attr_val_idx < sizeof(tok->current_attr_val) - 1) {
                        tok->current_attr_val[attr_val_idx++] = c;
                    }
                    tok->position++;
                }
                break;

            case STATE_ATTR_VAL_SINGLE_QUOTED:
                if (c == '\'') {
                    tok->current_attr_val[attr_val_idx] = '\0';
                    if (out_token->attribute_count < ABE_MAX_ATTRIBUTES) {
                        ABE_DOMAttributeInfo* a = &out_token->attributes[out_token->attribute_count++];
                        strncpy(a->name, tok->current_attr_name, sizeof(a->name) - 1);
                        ABE_HTMLTokenizer_DecodeEntities(tok->current_attr_val, a->value, sizeof(a->value));
                    }
                    tok->position++;
                    tok->state = STATE_BEFORE_ATTR_NAME;
                } else {
                    if (attr_val_idx < sizeof(tok->current_attr_val) - 1) {
                        tok->current_attr_val[attr_val_idx++] = c;
                    }
                    tok->position++;
                }
                break;

            case STATE_ATTR_VAL_UNQUOTED:
                if (IsWhitespace(c) || c == '>') {
                    tok->current_attr_val[attr_val_idx] = '\0';
                    if (out_token->attribute_count < ABE_MAX_ATTRIBUTES) {
                        ABE_DOMAttributeInfo* a = &out_token->attributes[out_token->attribute_count++];
                        strncpy(a->name, tok->current_attr_name, sizeof(a->name) - 1);
                        ABE_HTMLTokenizer_DecodeEntities(tok->current_attr_val, a->value, sizeof(a->value));
                    }
                    if (c == '>') {
                        tok->position++;
                        tok->state = STATE_DATA;
                        ABE_Diag_RecordHTMLToken();
                        return ABE_SUCCESS;
                    }
                    tok->position++;
                    tok->state = STATE_BEFORE_ATTR_NAME;
                } else {
                    if (attr_val_idx < sizeof(tok->current_attr_val) - 1) {
                        tok->current_attr_val[attr_val_idx++] = c;
                    }
                    tok->position++;
                }
                break;

            case STATE_SELF_CLOSING_TAG:
                if (c == '>') {
                    out_token->self_closing = true;
                    tok->position++;
                    tok->state = STATE_DATA;
                    ABE_Diag_RecordHTMLToken();
                    return ABE_SUCCESS;
                } else {
                    tok->state = STATE_BEFORE_ATTR_NAME;
                }
                break;

            case STATE_COMMENT:
                if (c == '>') {
                    out_token->type = TOKEN_COMMENT;
                    tok->position++;
                    tok->state = STATE_DATA;
                    ABE_Diag_RecordHTMLToken();
                    return ABE_SUCCESS;
                } else {
                    tok->position++;
                }
                break;

            default:
                tok->position++;
                tok->state = STATE_DATA;
                break;
        }
    }

    if (val_idx > 0) {
        out_token->type = TOKEN_CHARACTER;
        out_token->value[val_idx] = '\0';
        char decoded[512];
        ABE_HTMLTokenizer_DecodeEntities(out_token->value, decoded, sizeof(decoded));
        strncpy(out_token->value, decoded, sizeof(out_token->value) - 1);
        ABE_Diag_RecordHTMLToken();
        return ABE_SUCCESS;
    }

    out_token->type = TOKEN_EOF;
    return ABE_SUCCESS;
}
