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

static int CaseInsensitiveCompare(const char* s1, const char* s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s1[i] == '\0' || s2[i] == '\0') {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
        char c1 = ToLower(s1[i]);
        char c2 = ToLower(s2[i]);
        if (c1 != c2) return (unsigned char)c1 - (unsigned char)c2;
    }
    return 0;
}

static size_t EncodeUtf8(uint32_t cp, char* out) {
    if (cp <= 0x7F) {
        out[0] = (char)cp;
        return 1;
    } else if (cp <= 0x7FF) {
        out[0] = (char)(0xC0 | ((cp >> 6) & 0x1F));
        out[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    } else if (cp <= 0xFFFF) {
        out[0] = (char)(0xE0 | ((cp >> 12) & 0x0F));
        out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    } else if (cp <= 0x10FFFF) {
        out[0] = (char)(0xF0 | ((cp >> 18) & 0x07));
        out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[3] = (char)(0x80 | (cp & 0x3F));
        return 4;
    }
    return 0;
}

void ABE_HTMLTokenizer_DecodeEntities(const char* in_str, char* out_str, size_t max_len) {
    if (!in_str || !out_str || max_len == 0) return;

    size_t in_i = 0;
    size_t out_i = 0;

    while (in_str[in_i] != '\0' && out_i < max_len - 1) {
        if (in_str[in_i] == '&') {
            // Check numeric entities (&#65; or &#x41;)
            if (in_str[in_i + 1] == '#') {
                size_t num_start = in_i + 2;
                bool is_hex = false;
                if (in_str[num_start] == 'x' || in_str[num_start] == 'X') {
                    is_hex = true;
                    num_start++;
                }

                uint32_t val = 0;
                size_t p = num_start;
                bool valid_num = false;

                if (is_hex) {
                    while ((in_str[p] >= '0' && in_str[p] <= '9') ||
                           (in_str[p] >= 'a' && in_str[p] <= 'f') ||
                           (in_str[p] >= 'A' && in_str[p] <= 'F')) {
                        valid_num = true;
                        uint32_t digit = (in_str[p] >= '0' && in_str[p] <= '9') ? (in_str[p] - '0') :
                                         (in_str[p] >= 'a' && in_str[p] <= 'f') ? (in_str[p] - 'a' + 10) :
                                         (in_str[p] - 'A' + 10);
                        val = (val << 4) | digit;
                        p++;
                    }
                } else {
                    while (in_str[p] >= '0' && in_str[p] <= '9') {
                        valid_num = true;
                        val = val * 10 + (in_str[p] - '0');
                        p++;
                    }
                }

                if (valid_num && in_str[p] == ';') {
                    char utf8_seq[8];
                    size_t utf8_len = EncodeUtf8(val, utf8_seq);
                    if (utf8_len > 0 && out_i + utf8_len < max_len) {
                        for (size_t k = 0; k < utf8_len; k++) {
                            out_str[out_i++] = utf8_seq[k];
                        }
                        in_i = p + 1;
                        continue;
                    }
                }
            }

            // Named entities
            if (strncmp(in_str + in_i, "&amp;", 5) == 0) {
                out_str[out_i++] = '&'; in_i += 5;
            } else if (strncmp(in_str + in_i, "&lt;", 4) == 0) {
                out_str[out_i++] = '<'; in_i += 4;
            } else if (strncmp(in_str + in_i, "&gt;", 4) == 0) {
                out_str[out_i++] = '>'; in_i += 4;
            } else if (strncmp(in_str + in_i, "&quot;", 6) == 0) {
                out_str[out_i++] = '"'; in_i += 6;
            } else if (strncmp(in_str + in_i, "&apos;", 6) == 0 || strncmp(in_str + in_i, "&#39;", 5) == 0) {
                out_str[out_i++] = '\''; in_i += (in_str[in_i + 1] == '#' ? 5 : 6);
            } else if (strncmp(in_str + in_i, "&nbsp;", 6) == 0) {
                out_str[out_i++] = ' '; in_i += 6;
            } else if (strncmp(in_str + in_i, "&copy;", 6) == 0) {
                if (out_i + 2 < max_len) { out_str[out_i++] = '\xC2'; out_str[out_i++] = '\xA9'; in_i += 6; }
                else { out_str[out_i++] = in_str[in_i++]; }
            } else if (strncmp(in_str + in_i, "&reg;", 5) == 0) {
                if (out_i + 2 < max_len) { out_str[out_i++] = '\xC2'; out_str[out_i++] = '\xAE'; in_i += 5; }
                else { out_str[out_i++] = in_str[in_i++]; }
            } else if (strncmp(in_str + in_i, "&trade;", 7) == 0) {
                if (out_i + 3 < max_len) { out_str[out_i++] = '\xE2'; out_str[out_i++] = '\x84'; out_str[out_i++] = '\xA2'; in_i += 7; }
                else { out_str[out_i++] = in_str[in_i++]; }
            } else if (strncmp(in_str + in_i, "&mdash;", 7) == 0) {
                if (out_i + 3 < max_len) { out_str[out_i++] = '\xE2'; out_str[out_i++] = '\x80'; out_str[out_i++] = '\x94'; in_i += 7; }
                else { out_str[out_i++] = in_str[in_i++]; }
            } else if (strncmp(in_str + in_i, "&ndash;", 7) == 0) {
                if (out_i + 3 < max_len) { out_str[out_i++] = '\xE2'; out_str[out_i++] = '\x80'; out_str[out_i++] = '\x93'; in_i += 7; }
                else { out_str[out_i++] = in_str[in_i++]; }
            } else if (strncmp(in_str + in_i, "&bull;", 6) == 0) {
                if (out_i + 3 < max_len) { out_str[out_i++] = '\xE2'; out_str[out_i++] = '\x80'; out_str[out_i++] = '\xA2'; in_i += 6; }
                else { out_str[out_i++] = in_str[in_i++]; }
            } else {
                out_str[out_i++] = in_str[in_i++];
            }
        } else {
            out_str[out_i++] = in_str[in_i++];
        }
    }
    out_str[out_i] = '\0';
}

void ABE_HTMLTokenizer_SwitchToRawText(ABE_HTMLTokenizer* tok, const char* tag) {
    if (!tok || !tag) return;
    strncpy(tok->rawtext_tag, tag, sizeof(tok->rawtext_tag) - 1);
    tok->rawtext_end_tag_idx = 0;
    tok->state = STATE_RAWTEXT;
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
    size_t rawtext_tag_len = strlen(tok->rawtext_tag);

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
                    // Lookahead for comment <!-- or doctype <!DOCTYPE
                    if (tok->position + 3 <= tok->input_len &&
                        tok->input[tok->position + 1] == '-' && tok->input[tok->position + 2] == '-') {
                        tok->position += 3;
                        tok->state = STATE_COMMENT;
                    } else if (tok->position + 8 <= tok->input_len &&
                               CaseInsensitiveCompare(tok->input + tok->position + 1, "doctype", 7) == 0) {
                        tok->position += 8;
                        tok->state = STATE_BEFORE_DOCTYPE_NAME;
                    } else {
                        tok->position++;
                        tok->state = STATE_COMMENT;
                    }
                } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                    out_token->type = TOKEN_START_TAG;
                    tag_idx = 0;
                    tok->state = STATE_TAG_NAME;
                } else {
                    tok->state = STATE_DATA;
                }
                break;

            case STATE_BEFORE_DOCTYPE_NAME:
                if (IsWhitespace(c)) {
                    tok->position++;
                } else if (c == '>') {
                    tok->position++;
                    out_token->type = TOKEN_DOCTYPE;
                    strncpy(out_token->value, "html", sizeof(out_token->value) - 1);
                    tok->state = STATE_DATA;
                    ABE_Diag_RecordHTMLToken();
                    return ABE_SUCCESS;
                } else {
                    tag_idx = 0;
                    tok->state = STATE_DOCTYPE_NAME;
                }
                break;

            case STATE_DOCTYPE_NAME:
                if (IsWhitespace(c) || c == '>') {
                    out_token->value[tag_idx] = '\0';
                    out_token->type = TOKEN_DOCTYPE;
                    while (tok->position < tok->input_len && tok->input[tok->position] != '>') {
                        tok->position++;
                    }
                    if (tok->position < tok->input_len && tok->input[tok->position] == '>') {
                        tok->position++;
                    }
                    tok->state = STATE_DATA;
                    ABE_Diag_RecordHTMLToken();
                    return ABE_SUCCESS;
                } else {
                    if (tag_idx < sizeof(out_token->value) - 1) {
                        out_token->value[tag_idx++] = ToLower(c);
                    }
                    tok->position++;
                }
                break;

            case STATE_RAWTEXT:
                // Accumulate characters until matching closing tag
                if (c == '<' && tok->position + 2 + rawtext_tag_len <= tok->input_len &&
                    tok->input[tok->position + 1] == '/' &&
                    CaseInsensitiveCompare(tok->input + tok->position + 2, tok->rawtext_tag, rawtext_tag_len) == 0) {
                    // Check delimiter following tag name (whitespace, /, >)
                    char after_c = tok->input[tok->position + 2 + rawtext_tag_len];
                    if (after_c == '>' || after_c == '/' || IsWhitespace(after_c)) {
                        if (val_idx > 0) {
                            out_token->type = TOKEN_CHARACTER;
                            out_token->value[val_idx] = '\0';
                            tok->state = STATE_TAG_OPEN;
                            ABE_Diag_RecordHTMLToken();
                            return ABE_SUCCESS;
                        }
                        tok->position += 2; // skip </
                        tok->state = STATE_END_TAG_OPEN;
                        tok->rawtext_tag[0] = '\0';
                        break;
                    }
                }

                if (val_idx < sizeof(out_token->value) - 1) {
                    out_token->value[val_idx++] = c;
                }
                tok->position++;
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
                if (c == '-' && tok->position + 2 <= tok->input_len &&
                    tok->input[tok->position + 1] == '-' && tok->input[tok->position + 2] == '>') {
                    out_token->type = TOKEN_COMMENT;
                    out_token->value[val_idx] = '\0';
                    tok->position += 3;
                    tok->state = STATE_DATA;
                    ABE_Diag_RecordHTMLToken();
                    return ABE_SUCCESS;
                } else if (c == '>') {
                    out_token->type = TOKEN_COMMENT;
                    out_token->value[val_idx] = '\0';
                    tok->position++;
                    tok->state = STATE_DATA;
                    ABE_Diag_RecordHTMLToken();
                    return ABE_SUCCESS;
                } else {
                    if (val_idx < sizeof(out_token->value) - 1) {
                        out_token->value[val_idx++] = c;
                    }
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
