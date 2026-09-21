// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "vhdl/Parser.h"

#include <charconv>
#include <cstdint>
#include <string>

namespace vhdl {
namespace {

SourceSpan join(SourceSpan first, SourceSpan last) {
    return {first.start, last.end};
}

class RecursiveParser {
public:
    explicit RecursiveParser(std::string_view source) : lexed_(Lexer::scan(source)) {
        for (const auto& diagnostic : lexed_.diagnostics)
            result_.diagnostics.push_back({diagnostic.message, diagnostic.span});
    }

    ParseResult run() {
        while (!atEnd()) {
            const auto before = current().span.start.offset;
            if (word("entity")) {
                auto entity = parseEntity();
                if (entity)
                    result_.syntax.entities.push_back(std::move(*entity));
            }
            else if (word("architecture")) {
                auto architecture = parseArchitecture();
                if (architecture)
                    result_.syntax.architectures.push_back(std::move(*architecture));
            }
            else {
                error("expected an entity or architecture design unit", current().span);
                advance();
            }
            if (!atEnd() && current().span.start.offset == before)
                advance();
        }
        return std::move(result_);
    }

private:
    const Token& current() const { return lexed_.tokens[index_]; }
    const Token& look(std::size_t distance = 1) const {
        const auto next = index_ + distance;
        return lexed_.tokens[next < lexed_.tokens.size() ? next : lexed_.tokens.size() - 1];
    }
    bool atEnd() const { return current().kind == TokenKind::EndOfFile; }
    const Token& advance() {
        const auto& token = current();
        if (!atEnd())
            ++index_;
        return token;
    }
    bool word(std::string_view value) const {
        return current().kind == TokenKind::Identifier && current().canonical == value;
    }
    bool symbol(std::string_view value) const {
        return current().kind == TokenKind::Symbol && current().text == value;
    }
    bool acceptWord(std::string_view value) {
        if (!word(value))
            return false;
        advance();
        return true;
    }
    bool acceptSymbol(std::string_view value) {
        if (!symbol(value))
            return false;
        advance();
        return true;
    }
    bool expectWord(std::string_view value) {
        if (acceptWord(value))
            return true;
        error("expected keyword '" + std::string(value) + "'", current().span);
        return false;
    }
    bool expectSymbol(std::string_view value) {
        if (acceptSymbol(value))
            return true;
        error("expected '" + std::string(value) + "'", current().span);
        return false;
    }
    void error(std::string message, SourceSpan span) {
        result_.diagnostics.push_back({std::move(message), span});
    }
    static std::string_view nameKey(const Name& name) {
        return name.canonical.empty() ? std::string_view(name.spelling)
                                      : std::string_view(name.canonical);
    }
    std::optional<Name> parseName() {
        if (current().kind != TokenKind::Identifier &&
            current().kind != TokenKind::ExtendedIdentifier) {
            error("expected an identifier", current().span);
            return std::nullopt;
        }
        const auto token = advance();
        return Name{token.text, token.canonical, token.span};
    }
    bool isNameToken() const {
        return current().kind == TokenKind::Identifier ||
               current().kind == TokenKind::ExtendedIdentifier;
    }

    std::optional<EntityDeclaration> parseEntity() {
        const auto start = advance().span;
        auto name = parseName();
        if (!name || !expectWord("is"))
            return std::nullopt;
        EntityDeclaration entity{std::move(*name), {}, start};
        if (acceptWord("port")) {
            if (!expectSymbol("("))
                return std::nullopt;
            while (!atEnd() && !symbol(")")) {
                const auto before = current().span.start.offset;
                auto port = parsePortDeclaration();
                if (port)
                    entity.ports.push_back(std::move(*port));
                if (acceptSymbol(";"))
                    continue;
                if (!symbol(")")) {
                    error("expected ';' or ')' after port declaration", current().span);
                    synchronize("end");
                    return std::nullopt;
                }
                if (before == current().span.start.offset)
                    advance();
            }
            if (!expectSymbol(")") || !expectSymbol(";"))
                return std::nullopt;
        }
        if (!word("end")) {
            error("unsupported entity declaration; expected end of entity", current().span);
            synchronize("end");
        }
        if (!expectWord("end"))
            return std::nullopt;
        acceptWord("entity");
        if (isNameToken()) {
            auto closingName = parseName();
            if (closingName && nameKey(*closingName) != nameKey(entity.name))
                error("entity end name does not match its declaration", closingName->span);
        }
        if (!expectSymbol(";"))
            return std::nullopt;
        entity.span = join(start, lexed_.tokens[index_ - 1].span);
        return entity;
    }

    std::optional<PortDeclaration> parsePortDeclaration() {
        const auto start = current().span;
        std::vector<Name> names;
        auto name = parseName();
        if (!name)
            return std::nullopt;
        names.push_back(std::move(*name));
        while (acceptSymbol(",")) {
            name = parseName();
            if (!name)
                return std::nullopt;
            names.push_back(std::move(*name));
        }
        if (!expectSymbol(":"))
            return std::nullopt;
        PortMode mode = PortMode::In;
        if (acceptWord("in"))
            mode = PortMode::In;
        else if (acceptWord("out"))
            mode = PortMode::Out;
        else if (acceptWord("inout"))
            mode = PortMode::InOut;
        else if (acceptWord("buffer"))
            mode = PortMode::Buffer;
        else if (acceptWord("linkage"))
            mode = PortMode::Linkage;

        auto typeName = parseName();
        if (!typeName)
            return std::nullopt;
        TypeMark type{std::move(*typeName), std::nullopt};
        if (acceptSymbol("(")) {
            auto range = parseDiscreteRange();
            if (!range || !expectSymbol(")"))
                return std::nullopt;
            type.constraint = *range;
        }
        const auto end = type.constraint ? type.constraint->span : type.name.span;
        return PortDeclaration{std::move(names), mode, std::move(type), join(start, end)};
    }

    std::optional<std::int64_t> parseInteger() {
        const auto token = current();
        if (token.kind != TokenKind::IntegerLiteral || token.text.find('#') != std::string::npos) {
            error("expected a decimal integer bound", token.span);
            return std::nullopt;
        }
        advance();
        std::string digits;
        for (char c : token.text)
            if (c != '_')
                digits.push_back(c);
        std::int64_t value = 0;
        const auto [end, ec] = std::from_chars(digits.data(), digits.data() + digits.size(), value);
        if (ec != std::errc{} || end != digits.data() + digits.size()) {
            error("integer bound is outside the supported range", token.span);
            return std::nullopt;
        }
        return value;
    }

    std::optional<DiscreteRange> parseDiscreteRange() {
        const auto start = current().span;
        bool negativeLeft = acceptSymbol("-");
        if (!negativeLeft)
            acceptSymbol("+");
        auto left = parseInteger();
        if (!left || (!word("to") && !word("downto"))) {
            if (left)
                error("expected 'to' or 'downto' in range constraint", current().span);
            return std::nullopt;
        }
        if (negativeLeft)
            *left = -*left;
        const bool ascending = acceptWord("to");
        if (!ascending)
            acceptWord("downto");
        bool negativeRight = acceptSymbol("-");
        if (!negativeRight)
            acceptSymbol("+");
        auto right = parseInteger();
        if (!right)
            return std::nullopt;
        if (negativeRight)
            *right = -*right;
        return DiscreteRange{*left, *right, ascending, join(start, lexed_.tokens[index_ - 1].span)};
    }

    std::optional<ArchitectureBody> parseArchitecture() {
        const auto start = advance().span;
        auto name = parseName();
        if (!name || !expectWord("of"))
            return std::nullopt;
        auto entity = parseName();
        if (!entity || !expectWord("is"))
            return std::nullopt;
        ArchitectureBody architecture{std::move(*name), std::move(*entity), {}, {}, {}, start};
        while (acceptWord("signal")) {
            const auto signalStart = lexed_.tokens[index_ - 1].span;
            std::vector<Name> names;
            do {
                auto signalName = parseName();
                if (!signalName)
                    return std::nullopt;
                names.push_back(std::move(*signalName));
            } while (acceptSymbol(","));
            if (!expectSymbol(":"))
                return std::nullopt;
            auto typeName = parseName();
            if (!typeName)
                return std::nullopt;
            TypeMark type{std::move(*typeName), std::nullopt};
            if (acceptSymbol("(")) {
                auto range = parseDiscreteRange();
                if (!range || !expectSymbol(")"))
                    return std::nullopt;
                type.constraint = *range;
            }
            // Initializers, signal kinds, and other declaration semantics must
            // not be skipped: the proof has no representation for them.
            if (!expectSymbol(";"))
                return std::nullopt;
            architecture.signals.push_back({std::move(names), std::move(type),
                join(signalStart, lexed_.tokens[index_ - 1].span)});
        }
        if (!expectWord("begin"))
            return std::nullopt;
        while (!atEnd() && !word("end")) {
            const auto before = current().span.start.offset;
            if (word("process")) {
                auto process = parseClockedProcess();
                if (process)
                    architecture.processes.push_back(std::move(*process));
                else
                    synchronize("end");
            }
            else {
                auto assignment = parseConcurrentAssignment();
                if (assignment)
                    architecture.assignments.push_back(std::move(*assignment));
                else
                    synchronize("end");
            }
            if (!atEnd() && !word("end") && current().span.start.offset == before)
                advance();
        }
        if (!expectWord("end"))
            return std::nullopt;
        acceptWord("architecture");
        if (isNameToken()) {
            auto closingName = parseName();
            if (closingName && nameKey(*closingName) != nameKey(architecture.name))
                error("architecture end name does not match its declaration", closingName->span);
        }
        if (!expectSymbol(";"))
            return std::nullopt;
        architecture.span = join(start, lexed_.tokens[index_ - 1].span);
        return architecture;
    }

    std::optional<ClockedProcess> parseClockedProcess() {
        const auto start = advance().span;
        if (!expectSymbol("("))
            return std::nullopt;
        auto sensitivity = parseName();
        if (!sensitivity || !expectSymbol(")"))
            return std::nullopt;
        acceptWord("is");
        if (!expectWord("begin") || !expectWord("if"))
            return std::nullopt;
        auto eventSignal = parseName();
        if (!eventSignal || !expectSymbol("'") || !expectWord("event") ||
            !expectWord("and"))
            return std::nullopt;
        auto levelSignal = parseName();
        if (!levelSignal || !expectSymbol("="))
            return std::nullopt;
        const auto level = current();
        if (level.kind != TokenKind::CharacterLiteral) {
            error("expected a clock level character literal", level.span);
            return std::nullopt;
        }
        advance();
        if (!expectWord("then"))
            return std::nullopt;
        std::vector<ConcurrentAssignment> assignments;
        do {
            auto assignment = parseConcurrentAssignment();
            if (!assignment)
                return std::nullopt;
            assignments.push_back(std::move(*assignment));
        } while (!atEnd() && !word("end"));
        if (!expectWord("end") || !expectWord("if") ||
            !expectSymbol(";") || !expectWord("end") || !expectWord("process") ||
            !expectSymbol(";"))
            return std::nullopt;
        return ClockedProcess{std::move(*sensitivity), std::move(*eventSignal),
            std::move(*levelSignal), level.text, std::move(assignments),
            join(start, lexed_.tokens[index_ - 1].span)};
    }

    std::optional<ConcurrentAssignment> parseConcurrentAssignment() {
        const auto start = current().span;
        auto target = parseName();
        if (!target || !expectSymbol("<="))
            return std::nullopt;
        auto value = parseExpression(0);
        if (value && acceptWord("when")) {
            auto condition = parseExpression(0);
            if (!condition || !expectWord("else"))
                return std::nullopt;
            auto alternative = parseExpression(0);
            if (!alternative)
                return std::nullopt;
            auto conditional = std::make_unique<Expression>();
            conditional->kind = Expression::Kind::Conditional;
            conditional->span = join(value->span, alternative->span);
            conditional->left = std::move(value);
            conditional->right = std::move(alternative);
            conditional->condition = std::move(condition);
            value = std::move(conditional);
        }
        if (!value || !expectSymbol(";"))
            return std::nullopt;
        return ConcurrentAssignment{std::move(*target), std::move(value),
                                    join(start, lexed_.tokens[index_ - 1].span)};
    }

    int precedence() const {
        if (word("or") || word("nor") || word("xor") || word("xnor"))
            return 1;
        if (word("and") || word("nand"))
            return 2;
        if (symbol("=") || symbol("/=") || symbol("<") || symbol("<=") || symbol(">") ||
            symbol(">="))
            return 3;
        if (symbol("+") || symbol("-") || symbol("&"))
            return 4;
        if (symbol("*") || symbol("/") || word("mod") || word("rem"))
            return 5;
        if (symbol("**"))
            return 6;
        return -1;
    }

    std::unique_ptr<Expression> parseExpression(int minimumPrecedence) {
        auto left = parsePrimary();
        if (!left)
            return nullptr;
        while (precedence() >= minimumPrecedence) {
            const auto op = advance();
            const int opPrecedence = [&] {
                if (op.kind == TokenKind::Identifier) {
                    if (op.canonical == "or" || op.canonical == "nor" || op.canonical == "xor" ||
                        op.canonical == "xnor")
                        return 1;
                    if (op.canonical == "and" || op.canonical == "nand")
                        return 2;
                    if (op.canonical == "mod" || op.canonical == "rem")
                        return 5;
                }
                if (op.text == "=" || op.text == "/=" || op.text == "<" || op.text == "<=" ||
                    op.text == ">" || op.text == ">=")
                    return 3;
                if (op.text == "+" || op.text == "-" || op.text == "&")
                    return 4;
                if (op.text == "*" || op.text == "/")
                    return 5;
                if (op.text == "**")
                    return 6;
                return -1;
            }();
            auto right = parseExpression(opPrecedence + (op.text == "**" ? 0 : 1));
            if (!right)
                return nullptr;
            auto binary = std::make_unique<Expression>();
            binary->kind = Expression::Kind::Binary;
            binary->text = op.text;
            binary->span = join(left->span, right->span);
            binary->left = std::move(left);
            binary->right = std::move(right);
            left = std::move(binary);
        }
        return left;
    }

    std::unique_ptr<Expression> parsePrimary() {
        if (acceptSymbol("+") || acceptSymbol("-") || acceptWord("not")) {
            const auto& op = lexed_.tokens[index_ - 1];
            auto operand = parsePrimary();
            if (!operand)
                return nullptr;
            auto unary = std::make_unique<Expression>();
            unary->kind = Expression::Kind::Unary;
            unary->text = op.text.empty() ? op.canonical : op.text;
            unary->span = join(op.span, operand->span);
            unary->left = std::move(operand);
            return unary;
        }
        if (acceptSymbol("(")) {
            const auto start = lexed_.tokens[index_ - 1].span;
            auto nested = parseExpression(0);
            if (!nested || !expectSymbol(")"))
                return nullptr;
            nested->span = join(start, lexed_.tokens[index_ - 1].span);
            return nested;
        }
        if (isNameToken()) {
            auto name = parseName();
            auto expression = std::make_unique<Expression>();
            expression->kind = Expression::Kind::Name;
            expression->text = name->spelling;
            expression->canonical = name->canonical;
            expression->span = name->span;
            return expression;
        }
        const auto token = current();
        Expression::Kind kind;
        switch (token.kind) {
            case TokenKind::IntegerLiteral:
                kind = Expression::Kind::IntegerLiteral;
                break;
            case TokenKind::RealLiteral:
                kind = Expression::Kind::RealLiteral;
                break;
            case TokenKind::StringLiteral:
                kind = Expression::Kind::StringLiteral;
                break;
            case TokenKind::CharacterLiteral:
                kind = Expression::Kind::CharacterLiteral;
                break;
            default:
                error("expected a name, literal, or parenthesized expression", token.span);
                return nullptr;
        }
        advance();
        auto expression = std::make_unique<Expression>();
        expression->kind = kind;
        expression->text = token.text;
        expression->span = token.span;
        return expression;
    }

    void synchronize(std::string_view wordToStop) {
        while (!atEnd() && !word(wordToStop)) {
            if (symbol(";")) {
                advance();
                return;
            }
            advance();
        }
    }

    LexResult lexed_;
    ParseResult result_;
    std::size_t index_ = 0;
};

} // namespace

ParseResult Parser::parse(std::string_view source) {
    return RecursiveParser(source).run();
}

} // namespace vhdl
