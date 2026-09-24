// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "vhdl/Parser.h"

#include <charconv>
#include <cstdint>
#include <iterator>
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
            ContextClause context;
            while (word("library") || word("use")) {
                if (word("library")) {
                    auto clause = parseLibraryClause();
                    if (clause)
                        context.libraries.push_back(std::move(*clause));
                } else {
                    auto clauses = parseUseClause();
                    context.uses.insert(context.uses.end(),
                        std::make_move_iterator(clauses.begin()),
                        std::make_move_iterator(clauses.end()));
                }
            }
            if (word("entity")) {
                auto entity = parseEntity();
                if (entity) {
                    entity->context = std::move(context);
                    result_.syntax.entities.push_back(std::move(*entity));
                }
            }
            else if (word("architecture")) {
                auto architecture = parseArchitecture();
                if (architecture) {
                    architecture->context = std::move(context);
                    result_.syntax.architectures.push_back(std::move(*architecture));
                }
            }
            else if (word("package")) {
                auto package = parsePackage();
                if (package) {
                    package->context = std::move(context);
                    result_.syntax.packages.push_back(std::move(*package));
                }
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

    std::optional<LibraryClause> parseLibraryClause() {
        const auto start = advance().span;
        std::vector<Name> names;
        do {
            auto name = parseName();
            if (!name)
                return std::nullopt;
            names.push_back(std::move(*name));
        } while (acceptSymbol(","));
        if (!expectSymbol(";"))
            return std::nullopt;
        return LibraryClause{std::move(names),
            join(start, lexed_.tokens[index_ - 1].span)};
    }

    std::vector<UseClause> parseUseClause() {
        const auto start = advance().span;
        std::vector<UseClause> clauses;
        do {
            std::vector<Name> selectedName;
            auto name = parseName();
            if (!name)
                return clauses;
            selectedName.push_back(std::move(*name));
            while (acceptSymbol(".")) {
                name = parseName();
                if (!name)
                    return clauses;
                selectedName.push_back(std::move(*name));
            }
            clauses.push_back(UseClause{std::move(selectedName), start});
        } while (acceptSymbol(","));
        if (!expectSymbol(";"))
            return clauses;
        for (auto& clause : clauses)
            clause.span = join(start, lexed_.tokens[index_ - 1].span);
        return clauses;
    }

    std::optional<EntityDeclaration> parseEntity(bool component = false) {
        const auto start = advance().span;
        auto name = parseName();
        if (!name || (!component && !expectWord("is")))
            return std::nullopt;
        if (component) acceptWord("is");
        EntityDeclaration entity{std::move(*name), {}, {}, start};
        if (acceptWord("generic")) {
            if (!expectSymbol("("))
                return std::nullopt;
            while (!atEnd() && !symbol(")")) {
                auto generic = parseGenericDeclaration();
                if (generic)
                    entity.generics.push_back(std::move(*generic));
                if (acceptSymbol(";"))
                    continue;
                if (!symbol(")")) {
                    error("expected ';' or ')' after generic declaration", current().span);
                    synchronize("end");
                    return std::nullopt;
                }
            }
            if (!expectSymbol(")") || !expectSymbol(";"))
                return std::nullopt;
        }
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
        if (component) expectWord("component");
        else acceptWord("entity");
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

    std::optional<GenericDeclaration> parseGenericDeclaration() {
        const auto start = current().span;
        std::vector<Name> names;
        // Constants are the default interface class for entity generics.
        acceptWord("constant");
        do {
            auto name = parseName();
            if (!name)
                return std::nullopt;
            names.push_back(std::move(*name));
        } while (acceptSymbol(","));
        if (!expectSymbol(":"))
            return std::nullopt;
        if (word("in"))
            advance();
        auto typeName = parseName();
        if (!typeName)
            return std::nullopt;
        TypeMark type{std::move(*typeName), std::nullopt};
        if (acceptWord("range")) {
            auto range = parseDiscreteRange();
            if (!range) return std::nullopt;
            type.constraint = std::move(*range);
        }
        std::unique_ptr<Expression> defaultValue;
        if (acceptSymbol(":=")) {
            defaultValue = parseExpression(0);
            if (!defaultValue)
                return std::nullopt;
        }
        const auto end = defaultValue ? defaultValue->span : type.name.span;
        return GenericDeclaration{
            std::move(names), std::move(type), std::move(defaultValue), join(start, end)};
    }

    std::optional<PortDeclaration> parsePortDeclaration() {
        const auto start = current().span;
        acceptWord("signal");
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

    static std::optional<std::int64_t> decimalInteger(const Token& token) {
        if (token.kind != TokenKind::IntegerLiteral || token.text.find('#') != std::string::npos)
            return std::nullopt;
        std::string digits;
        for (char c : token.text)
            if (c != '_')
                digits.push_back(c);
        std::int64_t value = 0;
        const auto [end, ec] = std::from_chars(digits.data(), digits.data() + digits.size(), value);
        if (ec != std::errc{} || end != digits.data() + digits.size())
            return std::nullopt;
        return value;
    }

    static std::optional<std::int64_t> literalInteger(const Expression& expression) {
        if (expression.kind == Expression::Kind::IntegerLiteral) {
            Token token{TokenKind::IntegerLiteral, expression.text, {}, expression.span};
            return decimalInteger(token);
        }
        if (expression.kind != Expression::Kind::Unary || !expression.left)
            return std::nullopt;
        auto value = literalInteger(*expression.left);
        if (!value)
            return std::nullopt;
        if (expression.text == "-")
            return -*value;
        if (expression.text == "+")
            return *value;
        return std::nullopt;
    }

    std::optional<DiscreteRange> parseDiscreteRange() {
        const auto start = current().span;
        auto leftExpression = parseExpression(0);
        if (!leftExpression || (!word("to") && !word("downto"))) {
            if (leftExpression)
                error("expected 'to' or 'downto' in range constraint", current().span);
            return std::nullopt;
        }
        const bool ascending = acceptWord("to");
        if (!ascending)
            acceptWord("downto");
        auto rightExpression = parseExpression(0);
        if (!rightExpression)
            return std::nullopt;
        const auto left = literalInteger(*leftExpression).value_or(0);
        const auto right = literalInteger(*rightExpression).value_or(0);
        return DiscreteRange{left, right, ascending,
            join(start, lexed_.tokens[index_ - 1].span),
            std::shared_ptr<Expression>(std::move(leftExpression)),
            std::shared_ptr<Expression>(std::move(rightExpression))};
    }

    std::optional<ObjectDeclaration> parseObjectDeclaration(bool allowInitializer = false) {
        const auto start = lexed_.tokens[index_ - 1].span;
        std::vector<Name> names;
        do {
            auto name = parseName();
            if (!name)
                return std::nullopt;
            names.push_back(std::move(*name));
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
        if (acceptWord("range")) {
            auto range = parseDiscreteRange();
            if (!range) return std::nullopt;
            type.constraint = std::move(*range);
        }
        std::unique_ptr<Expression> initializer;
        if (allowInitializer && acceptSymbol(":=")) {
            initializer = parseExpression(0);
            if (!initializer) return std::nullopt;
        }
        if (!expectSymbol(";"))
            return std::nullopt;
        return ObjectDeclaration{std::move(names), std::move(type),
            join(start, lexed_.tokens[index_ - 1].span), std::move(initializer)};
    }

    std::optional<ArrayTypeDeclaration> parseArrayTypeDeclaration() {
        const auto start = lexed_.tokens[index_ - 1].span;
        auto name = parseName();
        if (!name || !expectWord("is") || !expectWord("array") ||
            !expectSymbol("("))
            return std::nullopt;
        std::optional<Name> indexSubtype;
        std::optional<DiscreteRange> indexRange;
        if (isNameToken() && look().canonical == "range" && look(2).text == "<>") {
            indexSubtype = parseName();
            advance(); advance();
            indexRange = DiscreteRange{};
        } else indexRange = parseDiscreteRange();
        if (!indexRange || !expectSymbol(")") || !expectWord("of"))
            return std::nullopt;
        auto elementName = parseName();
        if (!elementName)
            return std::nullopt;
        TypeMark elementType{std::move(*elementName), std::nullopt};
        if (acceptSymbol("(")) {
            auto range = parseDiscreteRange();
            if (!range || !expectSymbol(")"))
                return std::nullopt;
            elementType.constraint = std::move(*range);
        }
        if (!expectSymbol(";"))
            return std::nullopt;
        return ArrayTypeDeclaration{std::move(*name), std::move(*indexRange),
            std::move(elementType), join(start, lexed_.tokens[index_ - 1].span), std::move(indexSubtype)};
    }

    std::optional<PackageDeclaration> parsePackage() {
        advance();
        PackageDeclaration package;
        package.body = acceptWord("body");
        auto name = parseName();
        if (!name || !expectWord("is")) return std::nullopt;
        package.name = *name;
        while (!atEnd() && !word("end")) {
            if (!package.body && acceptWord("type")) {
                auto type = parseArrayTypeDeclaration();
                if (!type) return std::nullopt;
                package.arrayTypes.push_back(std::move(*type));
            } else if (!package.body && acceptWord("constant")) {
                auto constant = parseConstantDeclaration();
                if (!constant) return std::nullopt;
                package.constants.push_back(std::move(*constant));
            } else if (!package.body && word("component")) {
                auto component = parseEntity(true);
                if (!component) return std::nullopt;
                package.components.push_back(std::move(*component));
            } else {
                error("unsupported package declaration", current().span);
                return std::nullopt;
            }
        }
        if (!expectWord("end")) return std::nullopt;
        if (acceptWord("package") && package.body) expectWord("body");
        if (isNameToken()) {
            auto closing = parseName();
            if (nameKey(*closing) != nameKey(package.name))
                error("package end name mismatch", closing->span);
        }
        if (!expectSymbol(";")) return std::nullopt;
        return package;
    }

    std::optional<ConstantDeclaration> parseConstantDeclaration() {
        auto object = parseObjectDeclaration(true);
        if (!object) return std::nullopt;
        if (!object->initializer) {
            error("constant requires an initializer", object->span);
            return std::nullopt;
        }
        auto value = std::move(object->initializer);
        return ConstantDeclaration{std::move(*object), std::move(value)};
    }

    std::optional<GenerateStatement> parseGenerate() {
        GenerateStatement generate;
        if (isNameToken() && look().text == ":") {
            generate.label = *parseName();
            advance();
        }
        if (!expectWord("for")) return std::nullopt;
        auto iterator = parseName();
        if (!iterator || !expectWord("in")) return std::nullopt;
        auto range = parseDiscreteRange();
        if (!range || !expectWord("generate")) return std::nullopt;
        generate.iterator = *iterator;
        generate.range = std::move(*range);
        acceptWord("begin");
        while (!atEnd() && !word("end")) {
            if (word("for") || (isNameToken() && look().text == ":" && look(2).canonical == "for")) {
                auto child = parseGenerate();
                if (!child) return std::nullopt;
                generate.generates.push_back(std::move(*child));
            } else if (isNameToken() && look().text == ":") {
                auto instance = parseEntityInstantiation();
                if (!instance) return std::nullopt;
                generate.instantiations.push_back(std::move(*instance));
            } else {
                auto assignment = parseAssignment();
                if (!assignment) return std::nullopt;
                generate.assignments.push_back(std::move(*assignment));
            }
        }
        if (!expectWord("end") || !expectWord("generate")) return std::nullopt;
        if (isNameToken()) {
            auto closing = parseName();
            if (nameKey(*closing) != nameKey(generate.label)) {
                error("generate end name mismatch", closing->span);
                return std::nullopt;
            }
        }
        if (!expectSymbol(";")) return std::nullopt;
        return generate;
    }

    std::optional<ArchitectureBody> parseArchitecture() {
        const auto start = advance().span;
        auto name = parseName();
        if (!name || !expectWord("of"))
            return std::nullopt;
        auto entity = parseName();
        if (!entity || !expectWord("is"))
            return std::nullopt;
        ArchitectureBody architecture{
            std::move(*name), std::move(*entity), {}, {}, {}, {}, {}, start};
        while (word("signal") || word("type") || word("component") || word("constant")) {
            if (acceptWord("signal")) {
                auto declaration = parseObjectDeclaration(true);
                if (!declaration)
                    return std::nullopt;
                architecture.signals.push_back(std::move(*declaration));
            } else if (acceptWord("constant")) {
                auto declaration = parseConstantDeclaration();
                if (!declaration) return std::nullopt;
                architecture.constants.push_back(std::move(*declaration));
            } else if (word("component")) {
                auto component = parseEntity(true);
                if (!component) return std::nullopt;
                architecture.components.push_back(std::move(*component));
            } else {
                advance();
                auto declaration = parseArrayTypeDeclaration();
                if (!declaration)
                    return std::nullopt;
                architecture.arrayTypes.push_back(std::move(*declaration));
            }
        }
        // Do not report a valid, but unsupported, declarative item as a
        // missing `begin`.  Apart from being misleading, that diagnostic hides
        // which language feature must be implemented before the design can be
        // lowered faithfully.
        for (const auto declaration : {
                 "subtype", "variable", "shared", "file",
                 "alias", "attribute", "function", "procedure",
                 "package", "use", "group", "disconnect", "configuration"}) {
            if (word(declaration)) {
                error("architecture " + std::string(declaration) +
                      " declarations are not supported", current().span);
                return std::nullopt;
            }
        }
        if (!expectWord("begin"))
            return std::nullopt;
        while (!atEnd() && !word("end")) {
            const auto before = current().span.start.offset;
            if (isNameToken() && look().text == ":" &&
                look(2).canonical == "process") {
                advance(); advance();
            }
            if (word("for") || (isNameToken() && look().text == ":" && look(2).canonical == "for")) {
                auto generate = parseGenerate();
                if (!generate) return std::nullopt;
                architecture.generates.push_back(std::move(*generate));
            }
            else if (isNameToken() && look().text == ":") {
                auto instantiation = parseEntityInstantiation();
                if (instantiation)
                    architecture.instantiations.push_back(std::move(*instantiation));
                else
                    synchronize("end");
            }
            else if (word("process")) {
                auto process = parseClockedProcess();
                if (process)
                    architecture.processes.push_back(std::move(*process));
                else
                    synchronize("end");
            }
            else {
                auto assignment = parseAssignment();
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

    std::optional<EntityInstantiation> parseEntityInstantiation() {
        const auto start = current().span;
        auto label = parseName();
        if (!label || !expectSymbol(":"))
            return std::nullopt;
        const bool component = !acceptWord("entity");
        std::optional<Name> library = Name{"work", "work", start};
        if (!component) {
            library = parseName();
            if (!library || !expectSymbol(".")) return std::nullopt;
        }
        auto entity = parseName();
        if (!entity) return std::nullopt;
        std::optional<Name> architecture;
        if (acceptSymbol("(")) {
            architecture = parseName();
            if (!architecture || !expectSymbol(")"))
                return std::nullopt;
        }
        std::vector<GenericAssociation> generics;
        if (acceptWord("generic")) {
            if (!expectWord("map") || !expectSymbol("("))
                return std::nullopt;
            if (!symbol(")")) {
                do {
                    const auto associationStart = current().span;
                    std::optional<Name> formal;
                    if (isNameToken() && look().kind == TokenKind::Symbol &&
                        look().text == "=>") {
                        formal = parseName();
                        advance();
                    }
                    auto actual = parseExpression(0);
                    if (!actual)
                        return std::nullopt;
                    const auto associationEnd = actual->span;
                    generics.push_back({std::move(formal), std::move(actual),
                        join(associationStart, associationEnd)});
                } while (acceptSymbol(","));
            }
            if (!expectSymbol(")"))
                return std::nullopt;
        }
        if (!expectWord("port") || !expectWord("map") || !expectSymbol("("))
            return std::nullopt;
        std::vector<Name> actuals;
        std::vector<std::optional<Name>> formals;
        std::vector<std::vector<std::unique_ptr<Expression>>> actualIndices;
        if (!symbol(")")) {
            do {
                if (word("open")) {
                    error("open port associations are not supported", current().span);
                    return std::nullopt;
                }
                auto actual = parseName();
                if (!actual)
                    return std::nullopt;
                std::optional<Name> formal;
                if (acceptSymbol("=>")) {
                    formal = std::move(*actual);
                    actual = parseName();
                    if (!actual) return std::nullopt;
                }
                std::vector<std::unique_ptr<Expression>> indices;
                while (acceptSymbol("(")) {
                    auto index = parseIndex();
                    if (!index || !expectSymbol(")")) return std::nullopt;
                    indices.push_back(std::move(index));
                }
                actualIndices.push_back(std::move(indices));
                formals.push_back(std::move(formal));
                actuals.push_back(std::move(*actual));
            } while (acceptSymbol(","));
        }
        if (!expectSymbol(")") || !expectSymbol(";"))
            return std::nullopt;
        return EntityInstantiation{std::move(*label), std::move(*library),
            std::move(*entity), std::move(architecture), std::move(generics),
            std::move(actuals),
            join(start, lexed_.tokens[index_ - 1].span), std::move(formals), component,
            std::move(actualIndices)};
    }

    // Parse the restricted clock predicate separately from general expressions.
    // Parentheses may surround the guard or either event/level operand.
    struct ClockPredicate {
        std::optional<Name> eventSignal;
        std::optional<Name> levelSignal;
        std::string level;
        ClockEdgeForm form = ClockEdgeForm::EventAndLevel;
    };

    std::optional<ClockPredicate> parseClockPredicate(bool operand = false) {
        ClockPredicate predicate;
        if (acceptSymbol("(")) {
            auto nested = parseClockPredicate();
            if (!nested || !expectSymbol(")")) return std::nullopt;
            predicate = std::move(*nested);
        } else if (acceptWord("rising_edge")) {
            if (!expectSymbol("(")) return std::nullopt;
            predicate.eventSignal = parseName();
            if (!predicate.eventSignal || !expectSymbol(")")) return std::nullopt;
            predicate.levelSignal = predicate.eventSignal;
            predicate.level = "'1'";
            predicate.form = ClockEdgeForm::RisingEdgeCall;
        } else {
            auto signal = parseName();
            if (!signal) return std::nullopt;
            if (acceptSymbol("'")) {
                if (!expectWord("event")) return std::nullopt;
                predicate.eventSignal = std::move(signal);
            } else {
                if (!expectSymbol("=")) return std::nullopt;
                const auto token = current();
                if (token.kind != TokenKind::CharacterLiteral) {
                    error("expected a clock level character literal", token.span);
                    return std::nullopt;
                }
                advance();
                predicate.levelSignal = std::move(signal);
                predicate.level = token.text;
            }
        }
        if (!operand && acceptWord("and")) {
            auto right = parseClockPredicate(true);
            if (!right) return std::nullopt;
            if (predicate.form != ClockEdgeForm::EventAndLevel ||
                right->form != ClockEdgeForm::EventAndLevel ||
                (predicate.eventSignal && right->eventSignal) ||
                (predicate.levelSignal && right->levelSignal)) {
                error("expected one clock event and one clock level", current().span);
                return std::nullopt;
            }
            if (right->eventSignal) predicate.eventSignal = std::move(right->eventSignal);
            if (right->levelSignal) {
                predicate.levelSignal = std::move(right->levelSignal);
                predicate.level = std::move(right->level);
            }
        }
        return predicate;
    }

    bool parseClockGuard(ClockedProcess& process) {
        auto predicate = parseClockPredicate();
        if (!predicate) return false;
        if (!predicate->eventSignal || !predicate->levelSignal) {
            error("expected rising_edge or a clock event and level guard", current().span);
            return false;
        }
        process.eventSignal = std::move(*predicate->eventSignal);
        process.levelSignal = std::move(*predicate->levelSignal);
        process.level = std::move(predicate->level);
        process.edgeForm = predicate->form;
        return expectWord("then");
    }

    std::optional<ClockedProcess> parseClockedProcess() {
        const auto savedIndex = index_;
        const auto savedDiagnostics = result_.diagnostics.size();
        auto legacy = parseSimpleClockedProcess();
        if (legacy)
            return legacy;
        index_ = savedIndex;
        result_.diagnostics.resize(savedDiagnostics);
        const auto start = advance().span;
        ClockedProcess process;
        if (!expectSymbol("(")) return std::nullopt;
        do {
            auto name = parseName();
            if (!name) return std::nullopt;
            process.sensitivityList.push_back(std::move(*name));
        } while (acceptSymbol(","));
        if (!expectSymbol(")")) return std::nullopt;
        acceptWord("is");
        while (acceptWord("variable")) {
            auto declaration = parseObjectDeclaration();
            if (!declaration) return std::nullopt;
            process.variables.push_back(std::move(*declaration));
        }
        if (!expectWord("begin") || !expectWord("if") || !parseClockGuard(process))
            return std::nullopt;
        process.sensitivity = process.sensitivityList.front();
        if (word("end")) {
            error("expected a statement inside the clock guard", current().span);
            return std::nullopt;
        }
        if (!parseStatements(process.statements) || !expectWord("end") ||
            !expectWord("if") || !expectSymbol(";")) return std::nullopt;
        // Unconditional signal assignments outside the edge guard are kept
        // separately; lowering checks their sensitivity and scheduling.
        while (!atEnd() && !word("end")) {
            auto assignment = parseAssignment();
            if (!assignment) return std::nullopt;
            process.assignments.push_back(std::move(*assignment));
        }
        if (!expectWord("end") || !expectWord("process") || !expectSymbol(";"))
            return std::nullopt;
        process.span = join(start, lexed_.tokens[index_ - 1].span);
        return process;
    }

    bool parseStatements(std::vector<SequentialStatement>& statements) {
        while (!atEnd() && !word("end") && !word("else") && !word("elsif")) {
            SequentialStatement statement;
            const auto start = current().span;
            if (acceptWord("if")) {
                if (!parseIfStatement(statement)) return false;
            } else if (acceptWord("for")) {
                statement.kind = SequentialStatement::Kind::For;
                auto iterator = parseName();
                if (!iterator || !expectWord("in")) return false;
                auto range = parseDiscreteRange();
                if (!range || !expectWord("loop")) return false;
                statement.iterator = std::move(*iterator);
                statement.range = std::move(*range);
                if (!parseStatements(statement.statements) || !expectWord("end") ||
                    !expectWord("loop") || !expectSymbol(";")) return false;
            } else {
                auto assignment = parseAssignment(true);
                if (!assignment) return false;
                statement.assignment = std::move(*assignment);
            }
            statement.span = join(start, lexed_.tokens[index_ - 1].span);
            statements.push_back(std::move(statement));
        }
        return true;
    }

    bool parseIfStatement(SequentialStatement& statement) {
        statement.kind = SequentialStatement::Kind::If;
        statement.condition = parseExpression(0);
        if (!statement.condition || !expectWord("then") ||
            !parseStatements(statement.statements)) return false;
        if (acceptWord("elsif")) {
            SequentialStatement alternative;
            if (!parseIfStatement(alternative)) return false;
            statement.alternative.push_back(std::move(alternative));
            return true;
        }
        if (acceptWord("else") && !parseStatements(statement.alternative)) return false;
        return expectWord("end") && expectWord("if") && expectSymbol(";");
    }

    std::optional<ClockedProcess> parseSimpleClockedProcess() {
        const auto start = advance().span;
        if (!expectSymbol("("))
            return std::nullopt;
        auto sensitivity = parseName();
        if (!sensitivity || !expectSymbol(")"))
            return std::nullopt;
        acceptWord("is");
        std::vector<VariableDeclaration> variables;
        while (acceptWord("variable")) {
            auto declaration = parseObjectDeclaration();
            if (!declaration)
                return std::nullopt;
            variables.push_back(std::move(*declaration));
        }
        if (!expectWord("begin") || !expectWord("if"))
            return std::nullopt;
        ClockedProcess process;
        if (!parseClockGuard(process)) return std::nullopt;
        std::optional<Name> controlSignal;
        std::string controlLevel;
        if (acceptWord("if")) {
            controlSignal = parseName();
            if (!controlSignal || !expectSymbol("="))
                return std::nullopt;
            const auto controlToken = current();
            if (controlToken.kind != TokenKind::CharacterLiteral) {
                error("expected a control level character literal", controlToken.span);
                return std::nullopt;
            }
            advance();
            controlLevel = controlToken.text;
            if (!expectWord("then"))
                return std::nullopt;
        }
        std::vector<Assignment> assignments;
        do {
            auto assignment = parseAssignment(true);
            if (!assignment)
                return std::nullopt;
            assignments.push_back(std::move(*assignment));
        } while (!atEnd() && !word("end") && !word("else") && !word("elsif"));
        std::optional<Name> enableSignal;
        std::string enableLevel;
        std::optional<Name> resetSignal;
        std::string resetLevel;
        std::vector<Assignment> resetAssignments;
        if (controlSignal && acceptWord("elsif")) {
            resetSignal = std::move(controlSignal);
            resetLevel = std::move(controlLevel);
            resetAssignments = std::move(assignments);
            assignments.clear();
            enableSignal = parseName();
            if (!enableSignal || !expectSymbol("="))
                return std::nullopt;
            const auto enableToken = current();
            if (enableToken.kind != TokenKind::CharacterLiteral) {
                error("expected an enable level character literal", enableToken.span);
                return std::nullopt;
            }
            advance();
            enableLevel = enableToken.text;
            if (!expectWord("then"))
                return std::nullopt;
            do {
                auto assignment = parseAssignment(true);
                if (!assignment)
                    return std::nullopt;
                assignments.push_back(std::move(*assignment));
            } while (!atEnd() && !word("end"));
        } else if (controlSignal && acceptWord("else")) {
            resetSignal = std::move(controlSignal);
            resetLevel = std::move(controlLevel);
            resetAssignments = std::move(assignments);
            assignments.clear();
            do {
                auto assignment = parseAssignment(true);
                if (!assignment)
                    return std::nullopt;
                assignments.push_back(std::move(*assignment));
            } while (!atEnd() && !word("end"));
        } else if (controlSignal) {
            enableSignal = std::move(controlSignal);
            enableLevel = std::move(controlLevel);
        }
        if ((enableSignal || resetSignal) &&
            (!expectWord("end") || !expectWord("if") || !expectSymbol(";")))
            return std::nullopt;
        if (!expectWord("end") || !expectWord("if") ||
            !expectSymbol(";") || !expectWord("end") || !expectWord("process") ||
            !expectSymbol(";"))
            return std::nullopt;
        return ClockedProcess{std::move(*sensitivity), std::move(process.eventSignal),
            std::move(process.levelSignal), std::move(process.level), std::move(assignments),
            std::move(variables), std::move(enableSignal), std::move(enableLevel),
            std::move(resetSignal), std::move(resetLevel), std::move(resetAssignments), process.edgeForm,
            join(start, lexed_.tokens[index_ - 1].span)};
    }

    std::unique_ptr<Expression> parseIndex() {
        auto left = parseExpression(0);
        if (left && (word("to") || word("downto"))) {
            auto range = std::make_unique<Expression>();
            range->kind = Expression::Kind::Range;
            range->text = advance().canonical;
            range->left = std::move(left);
            range->right = parseExpression(0);
            if (!range->right) return nullptr;
            return range;
        }
        return left;
    }

    std::unique_ptr<Expression> parseConditional() {
        auto value = parseExpression(0);
        if (value && acceptWord("when")) {
            auto conditional = std::make_unique<Expression>();
            conditional->kind = Expression::Kind::Conditional;
            conditional->condition = parseExpression(0);
            if (!conditional->condition || !expectWord("else")) return nullptr;
            conditional->left = std::move(value);
            conditional->right = parseConditional();
            if (!conditional->right) return nullptr;
            conditional->span = join(conditional->left->span, conditional->right->span);
            return conditional;
        }
        return value;
    }

    std::optional<Assignment> parseAssignment(bool sequential = false) {
        const auto start = current().span;
        auto target = parseName();
        if (!target)
            return std::nullopt;
        std::vector<std::unique_ptr<Expression>> indices;
        while (acceptSymbol("(")) {
            auto index = parseIndex();
            if (!index || !expectSymbol(")")) return std::nullopt;
            indices.push_back(std::move(index));
        }
        const auto kind = sequential && acceptSymbol(":=")
            ? AssignmentKind::Variable : AssignmentKind::Signal;
        if (kind == AssignmentKind::Signal && !expectSymbol("<="))
            return std::nullopt;
        auto value = parseConditional();
        if (!value || !expectSymbol(";"))
            return std::nullopt;
        return Assignment{std::move(*target), std::move(value),
                                    join(start, lexed_.tokens[index_ - 1].span), kind,
                                    std::move(indices)};
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
            binary->text = op.kind == TokenKind::Identifier ? op.canonical : op.text;
            binary->span = join(left->span, right->span);
            binary->left = std::move(left);
            binary->right = std::move(right);
            left = std::move(binary);
        }
        return left;
    }

    std::unique_ptr<Expression> parsePrimary() {
        if (acceptSymbol("+") || acceptSymbol("-") || acceptWord("not") ||
            acceptWord("abs")) {
            const auto& op = lexed_.tokens[index_ - 1];
            auto operand = parsePrimary();
            if (!operand)
                return nullptr;
            auto unary = std::make_unique<Expression>();
            unary->kind = Expression::Kind::Unary;
            unary->text = op.kind == TokenKind::Identifier ? op.canonical : op.text;
            unary->span = join(op.span, operand->span);
            unary->left = std::move(operand);
            return unary;
        }
        if (acceptSymbol("(")) {
            const auto start = lexed_.tokens[index_ - 1].span;
            if (acceptWord("others")) {
                if (!expectSymbol("=>")) return nullptr;
                auto value = parseExpression(0);
                if (!value || !expectSymbol(")")) return nullptr;
                auto aggregate = std::make_unique<Expression>();
                aggregate->kind = Expression::Kind::Others;
                aggregate->left = std::move(value);
                aggregate->span = join(start, lexed_.tokens[index_ - 1].span);
                return aggregate;
            }
            auto nested = parseExpression(0);
            if (nested && acceptSymbol(",")) {
                auto aggregate = std::make_unique<Expression>();
                aggregate->kind = Expression::Kind::Aggregate;
                aggregate->elements.push_back(std::move(nested));
                do {
                    auto element = parseExpression(0);
                    if (!element) return nullptr;
                    aggregate->elements.push_back(std::move(element));
                } while (acceptSymbol(","));
                nested = std::move(aggregate);
            }
            if (!nested || !expectSymbol(")"))
                return nullptr;
            nested->span = join(start, lexed_.tokens[index_ - 1].span);
            return nested;
        }
        if ((word("x") || word("b") || word("o")) && look().kind == TokenKind::StringLiteral) {
            auto expression = std::make_unique<Expression>();
            expression->kind = Expression::Kind::BitStringLiteral;
            expression->canonical = advance().canonical;
            expression->text = advance().text;
            expression->span = lexed_.tokens[index_ - 1].span;
            return expression;
        }
        if (isNameToken()) {
            auto name = parseName();
            auto expression = std::make_unique<Expression>();
            expression->kind = Expression::Kind::Name;
            expression->text = name->spelling;
            expression->canonical = name->canonical;
            expression->span = name->span;
            while (acceptSymbol("(")) {
                auto index = parseIndex();
                if (!index) return nullptr;
                if (acceptSymbol(",")) {
                    auto call = std::make_unique<Expression>();
                    call->kind = Expression::Kind::Call;
                    call->left = std::move(expression);
                    call->elements.push_back(std::move(index));
                    do {
                        auto argument = parseExpression(0);
                        if (!argument) return nullptr;
                        call->elements.push_back(std::move(argument));
                    } while (acceptSymbol(","));
                    if (!expectSymbol(")")) return nullptr;
                    call->span = join(call->left->span, lexed_.tokens[index_ - 1].span);
                    expression = std::move(call);
                    continue;
                }
                if (!expectSymbol(")")) return nullptr;
                auto indexed = std::make_unique<Expression>();
                indexed->kind = Expression::Kind::Indexed;
                indexed->span = join(expression->span, lexed_.tokens[index_ - 1].span);
                indexed->left = std::move(expression);
                indexed->right = std::move(index);
                expression = std::move(indexed);
            }
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
