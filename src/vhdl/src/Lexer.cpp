// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "vhdl/Lexer.h"

#include <unordered_set>

namespace vhdl {
namespace {

class Scanner {
public:
    explicit Scanner(std::string_view source) : source_(source) {}

    LexResult run() {
        while (!atEnd()) {
            skipTrivia();
            if (atEnd())
                break;
            const auto start = position_;
            const char c = peek();
            if (isAlpha(c))
                scanIdentifier(start);
            else if (c == '\\')
                scanExtendedIdentifier(start);
            else if (isDigit(c))
                scanNumber(start);
            else if (c == '"')
                scanString(start);
            else if (c == '\'' && isCharacterLiteral())
                scanCharacter(start);
            else
                scanSymbol(start);
        }
        result_.tokens.push_back({TokenKind::EndOfFile, {}, {}, {position_, position_}});
        return std::move(result_);
    }

private:
    bool atEnd() const { return position_.offset >= source_.size(); }
    char peek(std::size_t lookahead = 0) const {
        const auto index = position_.offset + lookahead;
        return index < source_.size() ? source_[index] : '\0';
    }
    static bool isAlpha(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }
    static bool isDigit(char c) { return c >= '0' && c <= '9'; }
    static bool isAlphaNumeric(char c) { return isAlpha(c) || isDigit(c); }
    static bool isWhitespace(char c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
    }
    bool validDecimalSeparators(std::size_t start, std::size_t end) const {
        for (auto i = start; i < end; ++i) {
            if (source_[i] == '_' && (i == start || i + 1 == end || !isDigit(source_[i - 1]) ||
                                      !isDigit(source_[i + 1])))
                return false;
        }
        return true;
    }

    char advance() {
        const char c = source_[position_.offset++];
        if (c == '\n') {
            ++position_.line;
            position_.column = 1;
        }
        else {
            ++position_.column;
        }
        return c;
    }

    void emit(TokenKind kind, SourcePosition start, std::string canonical = {}) {
        const auto text = std::string(
            source_.substr(start.offset, position_.offset - start.offset));
        result_.tokens.push_back({kind, text, std::move(canonical), {start, position_}});
    }

    void diagnose(std::string message, SourcePosition start) {
        result_.diagnostics.push_back({std::move(message), {start, position_}});
    }

    void skipTrivia() {
        for (;;) {
            while (!atEnd() && isWhitespace(peek()))
                advance();
            if (peek() != '-')
                return;
            if (peek(1) != '-')
                return;
            while (!atEnd() && peek() != '\n')
                advance();
        }
    }

    void scanIdentifier(SourcePosition start) {
        while (isAlphaNumeric(peek()) || peek() == '_')
            advance();
        const auto text = source_.substr(start.offset, position_.offset - start.offset);
        bool valid = !text.empty() && text.back() != '_';
        for (std::size_t i = 1; i < text.size(); ++i) {
            if (text[i] == '_' && text[i - 1] == '_')
                valid = false;
        }
        if (!valid) {
            diagnose("malformed basic identifier", start);
            emit(TokenKind::Invalid, start);
            return;
        }
        std::string canonical(text);
        for (auto& c : canonical) {
            if (c >= 'A' && c <= 'Z')
                c = static_cast<char>(c - 'A' + 'a');
        }
        emit(TokenKind::Identifier, start, std::move(canonical));
    }

    void scanExtendedIdentifier(SourcePosition start) {
        advance();
        bool closed = false;
        while (!atEnd()) {
            if (peek() != '\\') {
                advance();
                continue;
            }
            advance();
            if (peek() == '\\') {
                advance();
                continue;
            }
            closed = true;
            break;
        }
        if (!closed) {
            diagnose("unterminated extended identifier", start);
            emit(TokenKind::Invalid, start);
            return;
        }
        emit(TokenKind::ExtendedIdentifier, start);
    }

    void scanNumber(SourcePosition start) {
        while (isDigit(peek()) || peek() == '_')
            advance();
        if (!validDecimalSeparators(start.offset, position_.offset)) {
            diagnose("underscores in a decimal literal must separate digits", start);
            emit(TokenKind::Invalid, start);
            return;
        }
        if (peek() == '#') {
            advance();
            bool real = false;
            const auto digitsStart = position_.offset;
            while (!atEnd() && peek() != '#') {
                if (peek() == '.')
                    real = true;
                advance();
            }
            if (peek() == '#')
                advance();
            else {
                diagnose("unterminated based literal", start);
                emit(TokenKind::Invalid, start);
                return;
            }
            for (auto i = digitsStart; i < position_.offset - 1; ++i) {
                if (source_[i] == '_' &&
                    (i == digitsStart || i + 1 == position_.offset - 1 ||
                     !isAlphaNumeric(source_[i - 1]) || !isAlphaNumeric(source_[i + 1]))) {
                    diagnose("underscores in a based literal must separate digits", start);
                    emit(TokenKind::Invalid, start);
                    return;
                }
            }
            if (peek() == 'e' || peek() == 'E') {
                real = true;
                advance();
                if (peek() == '+' || peek() == '-')
                    advance();
                const auto exponentStart = position_.offset;
                if (!isDigit(peek())) {
                    diagnose("based literal exponent requires at least one digit", start);
                    emit(TokenKind::Invalid, start);
                    return;
                }
                while (isDigit(peek()) || peek() == '_')
                    advance();
                if (!validDecimalSeparators(exponentStart, position_.offset)) {
                    diagnose("underscores in an exponent must separate digits", start);
                    emit(TokenKind::Invalid, start);
                    return;
                }
            }
            emit(real ? TokenKind::RealLiteral : TokenKind::IntegerLiteral, start);
            return;
        }
        bool real = false;
        if (peek() == '.' && isDigit(peek(1))) {
            real = true;
            advance();
            const auto fractionStart = position_.offset;
            while (isDigit(peek()) || peek() == '_')
                advance();
            if (!validDecimalSeparators(fractionStart, position_.offset)) {
                diagnose("underscores in a decimal fraction must separate digits", start);
                emit(TokenKind::Invalid, start);
                return;
            }
        }
        if (peek() == 'e' || peek() == 'E') {
            real = true;
            advance();
            if (peek() == '+' || peek() == '-')
                advance();
            const auto exponentStart = position_.offset;
            if (!isDigit(peek())) {
                diagnose("exponent requires at least one digit", start);
                emit(TokenKind::Invalid, start);
                return;
            }
            while (isDigit(peek()) || peek() == '_')
                advance();
            if (!validDecimalSeparators(exponentStart, position_.offset)) {
                diagnose("underscores in an exponent must separate digits", start);
                emit(TokenKind::Invalid, start);
                return;
            }
        }
        emit(real ? TokenKind::RealLiteral : TokenKind::IntegerLiteral, start);
    }

    void scanString(SourcePosition start) {
        advance();
        while (!atEnd()) {
            if (advance() != '"')
                continue;
            if (peek() == '"') {
                advance();
                continue;
            }
            emit(TokenKind::StringLiteral, start);
            return;
        }
        diagnose("unterminated string literal", start);
        emit(TokenKind::Invalid, start);
    }

    bool isCharacterLiteral() const {
        if (peek(1) == '\'' && peek(2) == '\'' && peek(3) == '\'')
            return true;
        return peek(1) != '\0' && peek(2) == '\'' && peek(1) != '\n';
    }

    void scanCharacter(SourcePosition start) {
        advance();
        advance();
        advance();
        if (source_.substr(start.offset, 3) == "'''" && peek() == '\'')
            advance();
        emit(TokenKind::CharacterLiteral, start);
    }

    void scanSymbol(SourcePosition start) {
        static const std::unordered_set<std::string> compound = {":=", "<=", ">=", "/=",
                                                                 "=>", "**", "<>"};
        static const std::string punctuation = "&'()*+,-./:;<=>|[]?";
        if (punctuation.find(peek()) == std::string::npos) {
            advance();
            diagnose("invalid character in VHDL source", start);
            emit(TokenKind::Invalid, start);
            return;
        }
        std::string candidate(1, peek());
        if (position_.offset + 1 < source_.size())
            candidate += peek(1);
        if (compound.contains(candidate))
            advance();
        advance();
        emit(TokenKind::Symbol, start);
    }

    std::string_view source_;
    SourcePosition position_;
    LexResult result_;
};

} // namespace

LexResult Lexer::scan(std::string_view source) {
    return Scanner(source).run();
}

} // namespace vhdl
