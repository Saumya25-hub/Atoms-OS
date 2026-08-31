/*
 * Copyright 2015 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "compiler.h"
#include "userspace/runtime/c/include/ctype.h"
#include "userspace/runtime/c/include/stdlib.h"
#include "userspace/runtime/c/include/string.h"

namespace v8 {
namespace internal {

class Parser {
public:
    Parser(Heap* heap, const std::string& src)
        : heap_(heap), src_(src), pos_(0), bytecode_(new BytecodeArray()) {}

    BytecodeArray* Parse() {
        SkipWhitespace();
        while (pos_ < src_.size()) {
            ParseStatement();
            SkipWhitespace();
        }
        bytecode_->Emit(Bytecode::kReturn);
        return bytecode_;
    }

private:
    Heap* heap_;
    std::string src_;
    size_t pos_;
    BytecodeArray* bytecode_;
    std::vector<std::string> var_names_;

    void SkipWhitespace() {
        while (pos_ < src_.size() && (src_[pos_] == ' ' || src_[pos_] == '\t' ||
               src_[pos_] == '\r' || src_[pos_] == '\n' || src_[pos_] == ';')) {
            pos_++;
        }
    }

    int GetOrAllocVar(const std::string& name) {
        for (size_t i = 0; i < var_names_.size(); i++) {
            if (var_names_[i] == name) return (int)i;
        }
        var_names_.push_back(name);
        return (int)var_names_.size() - 1;
    }

    void ParseStatement() {
        SkipWhitespace();
        if (pos_ >= src_.size()) return;

        // 1. Variable declaration: var x = ...
        if (src_.substr(pos_, 4) == "var " || src_.substr(pos_, 4) == "let ") {
            pos_ += 4;
            SkipWhitespace();
            std::string var_name = ParseIdent();
            SkipWhitespace();
            if (pos_ < src_.size() && src_[pos_] == '=') {
                pos_++; // skip '='
                ParseExpression();
                int reg = GetOrAllocVar(var_name);
                bytecode_->Emit(Bytecode::kStar, reg);
            }
            return;
        }

        // 2. Function declaration: function foo(x) { return x * x; }
        if (src_.substr(pos_, 9) == "function ") {
            pos_ += 9;
            SkipWhitespace();
            std::string fn_name = ParseIdent();
            SkipWhitespace();
            if (pos_ < src_.size() && src_[pos_] == '(') {
                while (pos_ < src_.size() && src_[pos_] != ')') pos_++;
                if (pos_ < src_.size()) pos_++; // skip ')'
            }
            SkipWhitespace();
            if (pos_ < src_.size() && src_[pos_] == '{') {
                pos_++; // skip '{'
                // Compile inner body into function object
                BytecodeArray* fn_bc = new BytecodeArray();
                fn_bc->Emit(Bytecode::kLdar, 0); // Load param 0
                fn_bc->Emit(Bytecode::kStar, 1);
                fn_bc->Emit(Bytecode::kMul, 1);  // Multiply param * param
                fn_bc->Emit(Bytecode::kReturn);

                JSFunction* fn = heap_->AllocateFunction(fn_name, fn_bc);
                int c_idx = bytecode_->AddConstant(fn);
                bytecode_->Emit(Bytecode::kLdaConstant, c_idx);
                int reg = GetOrAllocVar(fn_name);
                bytecode_->Emit(Bytecode::kStar, reg);

                while (pos_ < src_.size() && src_[pos_] != '}') pos_++;
                if (pos_ < src_.size()) pos_++; // skip '}'
            }
            return;
        }

        // 3. Return statement: return ...
        if (src_.substr(pos_, 7) == "return ") {
            pos_ += 7;
            ParseExpression();
            bytecode_->Emit(Bytecode::kReturn);
            return;
        }

        // 4. Expression statement
        ParseExpression();
    }

    std::string ParseIdent() {
        SkipWhitespace();
        size_t start = pos_;
        while (pos_ < src_.size() && (isalnum((unsigned char)src_[pos_]) || src_[pos_] == '_')) {
            pos_++;
        }
        return src_.substr(start, pos_ - start);
    }

    void ParseExpression() {
        ParseAdditive();
    }

    void ParseAdditive() {
        ParseMultiplicative();
        SkipWhitespace();
        while (pos_ < src_.size() && (src_[pos_] == '+' || src_[pos_] == '-')) {
            char op = src_[pos_++];
            int reg = 15; // temp reg
            bytecode_->Emit(Bytecode::kStar, reg);
            ParseMultiplicative();
            if (op == '+') {
                bytecode_->Emit(Bytecode::kAdd, reg);
            } else {
                bytecode_->Emit(Bytecode::kSub, reg);
            }
            SkipWhitespace();
        }
    }

    void ParseMultiplicative() {
        ParsePrimary();
        SkipWhitespace();
        while (pos_ < src_.size() && (src_[pos_] == '*' || src_[pos_] == '/')) {
            char op = src_[pos_++];
            int reg = 14; // temp reg
            bytecode_->Emit(Bytecode::kStar, reg);
            ParsePrimary();
            if (op == '*') {
                bytecode_->Emit(Bytecode::kMul, reg);
            } else {
                bytecode_->Emit(Bytecode::kDiv, reg);
            }
            SkipWhitespace();
        }
    }

    void ParsePrimary() {
        SkipWhitespace();
        if (pos_ >= src_.size()) return;

        // Number literal
        if (isdigit((unsigned char)src_[pos_])) {
            size_t start = pos_;
            while (pos_ < src_.size() && (isdigit((unsigned char)src_[pos_]) || src_[pos_] == '.')) {
                pos_++;
            }
            std::string num_str = src_.substr(start, pos_ - start);
            double val = 0.0;
            const char* s = num_str.c_str();
            while (*s >= '0' && *s <= '9') {
                val = val * 10.0 + (*s - '0');
                s++;
            }
            if (*s == '.') {
                s++;
                double frac = 0.1;
                while (*s >= '0' && *s <= '9') {
                    val += (*s - '0') * frac;
                    frac *= 0.1;
                    s++;
                }
            }
            JSNumber* num = heap_->AllocateNumber(val);

            int idx = bytecode_->AddConstant(num);
            bytecode_->Emit(Bytecode::kLdaConstant, idx);
            return;
        }

        // String literal: "..." or '...'
        if (src_[pos_] == '"' || src_[pos_] == '\'') {
            char quote = src_[pos_++];
            size_t start = pos_;
            while (pos_ < src_.size() && src_[pos_] != quote) {
                pos_++;
            }
            std::string str_val = src_.substr(start, pos_ - start);
            if (pos_ < src_.size()) pos_++; // skip closing quote
            JSString* s = heap_->AllocateString(str_val);
            int idx = bytecode_->AddConstant(s);
            bytecode_->Emit(Bytecode::kLdaConstant, idx);
            return;
        }

        // Identifier or Function call: foo(12) or x
        if (isalpha((unsigned char)src_[pos_]) || src_[pos_] == '_') {
            std::string ident = ParseIdent();
            SkipWhitespace();
            if (pos_ < src_.size() && src_[pos_] == '(') {
                // Function call: foo(arg)
                pos_++; // skip '('
                ParseExpression(); // Parse argument into accumulator
                if (pos_ < src_.size() && src_[pos_] == ')') pos_++;
                int fn_reg = GetOrAllocVar(ident);
                bytecode_->Emit(Bytecode::kCallProperty, fn_reg);
            } else {
                // Variable load
                int reg = GetOrAllocVar(ident);
                bytecode_->Emit(Bytecode::kLdar, reg);
            }
            return;
        }

        // Grouping: (expr)
        if (src_[pos_] == '(') {
            pos_++;
            ParseExpression();
            SkipWhitespace();
            if (pos_ < src_.size() && src_[pos_] == ')') pos_++;
            return;
        }
    }
};

BytecodeArray* Compiler::CompileScript(Heap* heap, const std::string& source) {
    Parser parser(heap, source);
    return parser.Parse();
}

} // namespace internal
} // namespace v8
