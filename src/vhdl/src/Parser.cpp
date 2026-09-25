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
    explicit RecursiveParser(std::string_view source, bool synthesis) : lexed_(Lexer::scan(source, synthesis)), synthesis_(synthesis) {
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
        if (acceptSymbol("(")) {
            auto range = parseDiscreteRange();
            if (!range || !expectSymbol(")")) return std::nullopt;
            type.constraint = std::move(*range);
        } else if (acceptWord("range")) {
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
        std::unique_ptr<Expression> defaultValue;
        if (acceptSymbol(":=")) {
            defaultValue = parseExpression(0);
            if (!defaultValue) return std::nullopt;
        }
        const auto end = defaultValue ? defaultValue->span :
            type.constraint ? type.constraint->span : type.name.span;
        return PortDeclaration{std::move(names), mode, std::move(type), join(start, end), std::move(defaultValue)};
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
        if (leftExpression && leftExpression->kind == Expression::Kind::Attribute &&
            (leftExpression->canonical == "range" || leftExpression->canonical == "reverse_range")) {
            DiscreteRange range;
            range.span = leftExpression->span;
            range.attribute = std::shared_ptr<Expression>(std::move(leftExpression));
            return range;
        }
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

    std::optional<EnumerationTypeDeclaration> parseEnumerationTypeDeclaration() {
        const auto start = lexed_.tokens[index_ - 1].span;
        auto name = parseName();
        if (!name || !expectWord("is") || !expectSymbol("(")) return std::nullopt;
        EnumerationTypeDeclaration type{*name, {}, start};
        do {
            auto literal = parseName();
            if (!literal) return std::nullopt;
            type.literals.push_back(std::move(*literal));
        } while (acceptSymbol(","));
        if (!expectSymbol(")") || !expectSymbol(";")) return std::nullopt;
        type.span = join(start, lexed_.tokens[index_ - 1].span);
        return type;
    }

    std::optional<RecordTypeDeclaration> parseRecordTypeDeclaration() {
        const auto start = lexed_.tokens[index_ - 1].span;
        auto name = parseName();
        if (!name || !expectWord("is") || !expectWord("record")) return std::nullopt;
        RecordTypeDeclaration record{*name, {}, start};
        while (!atEnd() && !word("end")) {
            auto field = parseObjectDeclaration();
            if (!field) return std::nullopt;
            record.fields.push_back(std::move(*field));
        }
        if (record.fields.empty()) {
            error("record requires at least one element", current().span);
            return std::nullopt;
        }
        if (!expectWord("end") || !expectWord("record")) return std::nullopt;
        if (isNameToken()) {
            auto closing = parseName();
            if (nameKey(*closing) != nameKey(*name)) {
                error("record end name mismatch", closing->span);
                return std::nullopt;
            }
        }
        if (!expectSymbol(";")) return std::nullopt;
        record.span = join(start, lexed_.tokens[index_ - 1].span);
        return record;
    }

    std::optional<ArrayTypeDeclaration> parseArrayTypeDeclaration() {
        const auto start = lexed_.tokens[index_ - 1].span;
        auto name = parseName();
        if (!name || !expectWord("is") || !expectWord("array") ||
            !expectSymbol("("))
            return std::nullopt;
        std::optional<Name> indexSubtype;
        std::optional<Name> indexType;
        std::optional<DiscreteRange> indexRange;
        if (isNameToken() && look().canonical == "range" && look(2).text == "<>") {
            indexSubtype = parseName();
            advance(); advance();
            indexRange = DiscreteRange{};
        } else if (isNameToken() && look().text == ")") {
            indexType = parseName();
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
            std::move(elementType), join(start, lexed_.tokens[index_ - 1].span), std::move(indexSubtype), std::move(indexType)};
    }

    std::optional<FunctionDeclaration> parseFunction() {
        const auto start = current().span;
        FunctionDeclaration function;
        function.pure = !acceptWord("impure");
        if (function.pure) acceptWord("pure");
        if (!expectWord("function")) return std::nullopt;
        auto name = parseName();
        if (!name) return std::nullopt;
        function.name = *name;
        if (acceptSymbol("(")) {
            do {
                auto parameter = parseGenericDeclaration();
                if (!parameter) return std::nullopt;
                function.parameters.push_back(std::move(*parameter));
            } while (acceptSymbol(";"));
            if (!expectSymbol(")")) return std::nullopt;
        }
        if (!expectWord("return")) return std::nullopt;
        auto result = parseName();
        if (!result) return std::nullopt;
        function.returnType = {std::move(*result), std::nullopt};
        if (acceptWord("is")) {
            function.body = true;
            while (word("variable") || word("constant")) {
                if (acceptWord("variable")) {
                    auto variable = parseObjectDeclaration(true);
                    if (!variable) return std::nullopt;
                    function.variables.push_back(std::move(*variable));
                } else {
                    advance();
                    auto constant = parseConstantDeclaration();
                    if (!constant) return std::nullopt;
                    function.constants.push_back(std::move(*constant));
                }
            }
            if (!expectWord("begin") || !parseStatements(function.statements, true) ||
                !expectWord("end")) return std::nullopt;
            acceptWord("function");
            if (isNameToken()) {
                auto closing = parseName();
                if (nameKey(*closing) != nameKey(function.name)) {
                    error("function end name mismatch", closing->span);
                    return std::nullopt;
                }
            }
        }
        if (!expectSymbol(";")) return std::nullopt;
        function.span = join(start, lexed_.tokens[index_ - 1].span);
        return function;
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
                if (look(2).text == "(") {
                    auto type = parseEnumerationTypeDeclaration();
                    if (!type) return std::nullopt;
                    package.enumerationTypes.push_back(std::move(*type));
                    continue;
                }
                if (look(2).canonical == "record") {
                    auto record = parseRecordTypeDeclaration();
                    if (!record) return std::nullopt;
                    package.recordTypes.push_back(std::move(*record));
                    continue;
                }
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
            } else if (word("function") || word("pure") || word("impure")) {
                auto function = parseFunction();
                if (!function) return std::nullopt;
                if (package.body && !function->body) {
                    error("package body requires a function body", function->span);
                    return std::nullopt;
                }
                package.functions.push_back(std::move(*function));
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

    bool startsGenerate() const {
        return word("for") || word("if") || (isNameToken() && look().text == ":" &&
            (look(2).canonical == "for" || look(2).canonical == "if"));
    }

    bool startsIgnoredDiagnostic() const {
        if (!synthesis_) return false;
        auto offset = isNameToken() && look().text == ":" ? 2u : 0u;
        if (look(offset).canonical == "postponed") ++offset;
        return look(offset).canonical == "assert" || look(offset).canonical == "report";
    }

    bool parseIgnoredDiagnostic() {
        const auto start = current().span;
        if (isNameToken() && look().text == ":") { advance(); advance(); }
        acceptWord("postponed");
        const bool assertion = acceptWord("assert");
        if (assertion && !parseExpression(0)) return false;
        if (!assertion || word("report")) {
            if (!expectWord("report") || !parseExpression(0)) return false;
        }
        if (acceptWord("severity") && !parseExpression(0)) return false;
        if (!expectSymbol(";")) return false;
        result_.warnings.push_back({
            assertion ? "ignored-assertion" : "ignored-report",
            assertion ? "assertion ignored for synthesis; condition, report, and severity are not evaluated"
                      : "report ignored for synthesis; message and severity are not evaluated",
            join(start, lexed_.tokens[index_ - 1].span)});
        return true;
    }

    bool parseGenerateBody(GenerateStatement& generate) {
        bool declarations = false;
        while (word("signal") || word("constant") || word("type")) {
            declarations = true;
            if (acceptWord("signal")) {
                auto signal = parseObjectDeclaration(true);
                if (!signal) return false;
                generate.signals.push_back(std::move(*signal));
            } else if (acceptWord("constant")) {
                auto constant = parseConstantDeclaration();
                if (!constant) return false;
                generate.constants.push_back(std::move(*constant));
            } else {
                advance();
                if (look(2).text == "(") {
                    auto type = parseEnumerationTypeDeclaration();
                    if (!type) return false;
                    generate.enumerationTypes.push_back(std::move(*type));
                } else if (look(2).canonical == "record") {
                    auto record = parseRecordTypeDeclaration();
                    if (!record) return false;
                    generate.recordTypes.push_back(std::move(*record));
                } else {
                    auto array = parseArrayTypeDeclaration();
                    if (!array) return false;
                    generate.arrayTypes.push_back(std::move(*array));
                }
            }
        }
        for (const auto declaration : {"subtype", "function", "procedure", "component", "attribute", "shared", "file", "alias", "use"}) {
            if (word(declaration)) {
                error("generate-local " + std::string(declaration) + " declarations are not supported", current().span);
                return false;
            }
        }
        if (declarations) {
            if (!expectWord("begin")) return false;
        } else acceptWord("begin");
        while (!atEnd() && !word("end") && !word("elsif") && !word("else")) {
            std::optional<Name> processLabel;
            if (isNameToken() && look().text == ":" && look(2).canonical == "process") {
                processLabel = parseName();
                advance();
            }
            if (startsIgnoredDiagnostic()) {
                if (!parseIgnoredDiagnostic()) return false;
                continue;
            }
            if (startsGenerate()) {
                auto child = parseGenerate();
                if (!child) return false;
                generate.generates.push_back(std::move(*child));
            } else if (word("process")) {
                auto process = parseClockedProcess(processLabel);
                if (!process) return false;
                generate.processes.push_back(std::move(*process));
            } else if (isNameToken() && look().text == ":") {
                auto instance = parseEntityInstantiation();
                if (!instance) return false;
                generate.instantiations.push_back(std::move(*instance));
            } else {
                auto assignment = parseAssignment(false, true);
                if (!assignment) return false;
                generate.assignments.push_back(std::move(*assignment));
            }
        }
        return true;
    }

    std::optional<GenerateStatement> parseGenerate() {
        GenerateStatement generate;
        if (isNameToken() && look().text == ":") {
            generate.label = *parseName();
            advance();
        }
        if (acceptWord("if")) {
            generate.conditional = true;
            generate.condition = parseExpression(0);
            if (!generate.condition) return std::nullopt;
        } else {
            if (!expectWord("for")) return std::nullopt;
            auto iterator = parseName();
            if (!iterator || !expectWord("in")) return std::nullopt;
            auto range = parseDiscreteRange();
            if (!range) return std::nullopt;
            generate.iterator = *iterator;
            generate.range = std::move(*range);
        }
        if (!expectWord("generate") || !parseGenerateBody(generate)) return std::nullopt;
        if (generate.conditional) {
            while (word("elsif") || word("else")) {
                const bool otherwise = acceptWord("else");
                GenerateStatement branch;
                branch.conditional = true;
                if (!otherwise) {
                    advance();
                    branch.condition = parseExpression(0);
                    if (!branch.condition) return std::nullopt;
                }
                if (!expectWord("generate") || !parseGenerateBody(branch)) return std::nullopt;
                generate.alternatives.push_back(std::move(branch));
                if (otherwise) break;
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
        while (word("signal") || word("type") || word("component") || word("constant") ||
               word("function") || word("pure") || word("impure")) {
            if (word("function") || word("pure") || word("impure")) {
                auto function = parseFunction();
                if (!function) return std::nullopt;
                architecture.functions.push_back(std::move(*function));
                continue;
            }
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
                if (look(2).text == "(") {
                    auto type = parseEnumerationTypeDeclaration();
                    if (!type) return std::nullopt;
                    architecture.enumerationTypes.push_back(std::move(*type));
                    continue;
                }
                if (look(2).canonical == "record") {
                    auto record = parseRecordTypeDeclaration();
                    if (!record) return std::nullopt;
                    architecture.recordTypes.push_back(std::move(*record));
                    continue;
                }
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
                 "alias", "attribute", "procedure",
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
            std::optional<Name> processLabel;
            if (isNameToken() && look().text == ":" &&
                look(2).canonical == "process") {
                processLabel = parseName();
                advance();
            }
            if (startsIgnoredDiagnostic()) {
                if (!parseIgnoredDiagnostic()) return std::nullopt;
                continue;
            }
            if (startsGenerate()) {
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
                auto process = parseClockedProcess(processLabel);
                if (process)
                    architecture.processes.push_back(std::move(*process));
                else
                    synchronize("end");
            }
            else {
                auto assignment = parseAssignment(false, true);
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

    bool startsNamedPortAssociation() const {
        if (!isNameToken()) return false;
        size_t depth = 0;
        for (size_t distance = 1; index_ + distance < lexed_.tokens.size(); ++distance) {
            const auto& token = look(distance);
            if (token.text == "=>" && depth == 0) return true;
            if (token.text == "(") ++depth;
            else if (token.text == ")") {
                if (depth == 0) return false;
                --depth;
            } else if ((token.text == "," && depth == 0) || token.text == ";") return false;
        }
        return false;
    }

    bool parsePortSelection(std::vector<std::unique_ptr<Expression>>& indices) {
        while (symbol(".") || symbol("(")) {
            if (acceptSymbol(".")) {
                auto field = parseName();
                if (!field) return false;
                auto selected = std::make_unique<Expression>();
                selected->kind = Expression::Kind::Selected;
                selected->text = field->spelling;
                selected->canonical = field->canonical;
                selected->span = field->span;
                indices.push_back(std::move(selected));
            } else {
                advance();
                auto index = parseIndex();
                if (!index || !expectSymbol(")")) return false;
                indices.push_back(std::move(index));
            }
        }
        return true;
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
        std::vector<std::unique_ptr<Expression>> actualLiterals;
        std::vector<std::vector<std::unique_ptr<Expression>>> formalIndices;
        std::vector<bool> actualOpen;
        if (!symbol(")")) {
            do {
                std::optional<Name> formal;
                formalIndices.emplace_back();
                if (startsNamedPortAssociation()) {
                    formal = parseName();
                    if (!formal || !parsePortSelection(formalIndices.back()) ||
                        !expectSymbol("=>")) return std::nullopt;
                }
                actualOpen.push_back(word("open"));
                if (acceptWord("open")) {
                    actualLiterals.push_back(nullptr);
                    actualIndices.emplace_back();
                    formals.push_back(std::move(formal));
                    actuals.push_back(Name{{}, {}, lexed_.tokens[index_ - 1].span});
                    continue;
                }
                if (current().kind == TokenKind::CharacterLiteral ||
                    current().kind == TokenKind::StringLiteral ||
                    ((word("x") || word("b") || word("o")) && look().kind == TokenKind::StringLiteral)) {
                    auto literal = parseExpression(0);
                    if (!literal) return std::nullopt;
                    if (literal->kind != Expression::Kind::CharacterLiteral &&
                        literal->kind != Expression::Kind::StringLiteral &&
                        literal->kind != Expression::Kind::BitStringLiteral) {
                        error("port-map expressions other than literals are unsupported", literal->span);
                        return std::nullopt;
                    }
                    actualLiterals.push_back(std::move(literal));
                    actualIndices.emplace_back();
                    formals.push_back(std::move(formal));
                    actuals.emplace_back();
                    continue;
                }
                actualLiterals.push_back(nullptr);
                auto actual = parseName();
                if (!actual) return std::nullopt;
                std::vector<std::unique_ptr<Expression>> indices;
                if (!parsePortSelection(indices)) return std::nullopt;
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
            std::move(actualIndices), std::move(actualLiterals), std::move(formalIndices), std::move(actualOpen)};
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

    bool parseProcessEnd(const std::optional<Name>& label) {
        if (!expectWord("end") || !expectWord("process")) return false;
        if (isNameToken()) {
            auto closing = parseName();
            if (!closing) return false;
            if (!label || nameKey(*closing) != nameKey(*label)) {
                error("process end label does not match its declaration", closing->span);
                return false;
            }
        }
        return expectSymbol(";");
    }

    std::optional<ClockedProcess> parseClockedProcess(const std::optional<Name>& label) {
        // Keep edge-process diagnostics for malformed clock/reset idioms.
        for (auto i = index_ + 1; i < lexed_.tokens.size(); ++i) {
            const auto& token = lexed_.tokens[i];
            if (token.canonical == "process" && lexed_.tokens[i - 1].canonical == "end") break;
            if (((token.canonical == "rising_edge" || token.canonical == "falling_edge") &&
                 i + 1 < lexed_.tokens.size() && lexed_.tokens[i + 1].text == "(") ||
                (token.canonical == "event" && lexed_.tokens[i - 1].text == "'"))
            {
                auto process = parseEdgeProcess(label);
                if (process) process->label = label;
                return process;
            }
        }
        const auto start = advance().span;
        ClockedProcess process;
        process.combinational = true;
        process.label = label;
        if (!expectSymbol("(")) return std::nullopt;
        if (acceptWord("all")) process.allSensitivity = true;
        else do {
            auto name = parseName();
            if (!name) return std::nullopt;
            process.sensitivityList.push_back(std::move(*name));
            std::vector<Name> fields;
            while (acceptSymbol(".")) {
                auto field = parseName();
                if (!field) return std::nullopt;
                fields.push_back(std::move(*field));
            }
            process.sensitivityFields.push_back(std::move(fields));
        } while (acceptSymbol(","));
        if (!expectSymbol(")")) return std::nullopt;
        acceptWord("is");
        while (acceptWord("variable")) {
            auto declaration = parseObjectDeclaration();
            if (!declaration) return std::nullopt;
            process.variables.push_back(std::move(*declaration));
        }
        if (!expectWord("begin") || !parseStatements(process.statements) ||
            !parseProcessEnd(label)) return std::nullopt;
        process.span = join(start, lexed_.tokens[index_ - 1].span);
        return process;
    }

    std::optional<ClockedProcess> parseEdgeProcess(const std::optional<Name>& label) {
        const auto savedIndex = index_;
        const auto savedDiagnostics = result_.diagnostics.size();
        auto legacy = parseSimpleClockedProcess(label);
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
            std::vector<Name> fields;
            while (acceptSymbol(".")) {
                auto field = parseName();
                if (!field) return std::nullopt;
                fields.push_back(std::move(*field));
            }
            process.sensitivityFields.push_back(std::move(fields));
        } while (acceptSymbol(","));
        if (!expectSymbol(")")) return std::nullopt;
        acceptWord("is");
        while (acceptWord("variable")) {
            auto declaration = parseObjectDeclaration();
            if (!declaration) return std::nullopt;
            process.variables.push_back(std::move(*declaration));
        }
        if (!expectWord("begin") || !expectWord("if")) return std::nullopt;
        const auto guardIndex = index_;
        const auto guardDiagnostics = result_.diagnostics.size();
        if (!parseClockGuard(process)) {
            index_ = guardIndex;
            result_.diagnostics.resize(guardDiagnostics);
            auto reset = parseExpression(0);
            if (!reset || reset->kind != Expression::Kind::Binary || reset->text != "=") {
                error("expected a clock guard or scalar reset equality", current().span);
                return std::nullopt;
            }
            const auto* signal = reset->left.get();
            const auto* level = reset->right.get();
            if (signal->kind == Expression::Kind::CharacterLiteral) std::swap(signal, level);
            if (signal->kind != Expression::Kind::Name || level->kind != Expression::Kind::CharacterLiteral ||
                (level->text != "'0'" && level->text != "'1'")) {
                error("asynchronous reset must compare a signal to '0' or '1'", reset->span);
                return std::nullopt;
            }
            process.asynchronousReset = true;
            process.resetSignal = Name{signal->text, signal->canonical, signal->span};
            process.resetLevel = level->text;
            if (!expectWord("then") || !parseStatements(process.resetStatements) ||
                !expectWord("elsif") || !parseClockGuard(process)) return std::nullopt;
        }
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
        if (!parseProcessEnd(label))
            return std::nullopt;
        process.span = join(start, lexed_.tokens[index_ - 1].span);
        return process;
    }

    bool parseStatements(std::vector<SequentialStatement>& statements, bool functionBody = false) {
        while (!atEnd() && !word("end") && !word("else") && !word("elsif") && !word("when")) {
            if (word("wait")) {
                error(current().canonical + " statements are not supported", current().span);
                return false;
            }
            SequentialStatement statement;
            const auto start = current().span;
            if (startsIgnoredDiagnostic()) {
                if (!parseIgnoredDiagnostic()) return false;
                statement.kind = SequentialStatement::Kind::Null;
            } else if (acceptWord("exit")) {
                if (!functionBody) {
                    error("exit is currently supported only in function loops", start);
                    return false;
                }
                statement.kind = SequentialStatement::Kind::Exit;
                if (acceptWord("when")) {
                    statement.condition = parseExpression(0);
                    if (!statement.condition) return false;
                } else if (!symbol(";")) {
                    error("labeled exit is not supported", current().span);
                    return false;
                }
                if (!expectSymbol(";")) return false;
            } else if (acceptWord("null")) {
                statement.kind = SequentialStatement::Kind::Null;
                if (!expectSymbol(";")) return false;
            } else if (acceptWord("case")) {
                statement.kind = SequentialStatement::Kind::Case;
                statement.condition = parseExpression(0);
                if (!statement.condition || !expectWord("is")) return false;
                do {
                    if (!expectWord("when")) return false;
                    std::vector<std::unique_ptr<Expression>> choices;
                    const bool otherwise = acceptWord("others");
                    if (!otherwise) do {
                        auto choice = parseIndex();
                        if (!choice) return false;
                        choices.push_back(std::move(choice));
                    } while (acceptSymbol("|"));
                    if (!expectSymbol("=>")) return false;
                    std::vector<SequentialStatement> body;
                    if (!parseStatements(body, functionBody) || body.empty()) {
                        error("case alternative requires a statement", current().span);
                        return false;
                    }
                    statement.choices.push_back(std::move(choices));
                    statement.caseBodies.push_back(std::move(body));
                    if (otherwise) break;
                } while (word("when"));
                if (!expectWord("end") || !expectWord("case") || !expectSymbol(";")) return false;
            } else if (acceptWord("if")) {
                if (!parseIfStatement(statement, functionBody)) return false;
            } else if (acceptWord("for")) {
                statement.kind = SequentialStatement::Kind::For;
                auto iterator = parseName();
                if (!iterator || !expectWord("in")) return false;
                auto range = parseDiscreteRange();
                if (!range || !expectWord("loop")) return false;
                statement.iterator = std::move(*iterator);
                statement.range = std::move(*range);
                if (!parseStatements(statement.statements, functionBody) || !expectWord("end") ||
                    !expectWord("loop") || !expectSymbol(";")) return false;
            } else if (functionBody && acceptWord("return")) {
                statement.kind = SequentialStatement::Kind::Return;
                statement.returnValue = parseExpression(0);
                if (!statement.returnValue || !expectSymbol(";")) return false;
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

    bool parseIfStatement(SequentialStatement& statement, bool functionBody = false) {
        statement.kind = SequentialStatement::Kind::If;
        statement.condition = parseExpression(0);
        if (!statement.condition || !expectWord("then") ||
            !parseStatements(statement.statements, functionBody)) return false;
        if (acceptWord("elsif")) {
            SequentialStatement alternative;
            if (!parseIfStatement(alternative, functionBody)) return false;
            statement.alternative.push_back(std::move(alternative));
            return true;
        }
        if (acceptWord("else") && !parseStatements(statement.alternative, functionBody)) return false;
        return expectWord("end") && expectWord("if") && expectSymbol(";");
    }

    std::optional<ClockedProcess> parseSimpleClockedProcess(const std::optional<Name>& label) {
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
        } while (!atEnd() && !word("end") && !word("else") && !word("elsif") && !word("when"));
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
            !expectSymbol(";") || !parseProcessEnd(label))
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

    std::optional<Assignment> parseAssignment(bool sequential = false, bool selectedAllowed = false) {
        const auto start = current().span;
        std::unique_ptr<Expression> selector;
        if (selectedAllowed && acceptWord("with")) {
            selector = parseExpression(0);
            if (!selector || !expectWord("select")) return std::nullopt;
        }
        auto target = parseName();
        if (!target)
            return std::nullopt;
        std::vector<std::unique_ptr<Expression>> indices;
        while (symbol(".") || symbol("(")) {
            if (acceptSymbol(".")) {
                auto field = parseName();
                if (!field) return std::nullopt;
                auto selected = std::make_unique<Expression>();
                selected->kind = Expression::Kind::Selected;
                selected->text = field->spelling;
                selected->canonical = field->canonical;
                selected->span = field->span;
                indices.push_back(std::move(selected));
                continue;
            }
            advance();
            auto index = parseIndex();
            if (!index || !expectSymbol(")")) return std::nullopt;
            indices.push_back(std::move(index));
        }
        const auto kind = sequential && acceptSymbol(":=")
            ? AssignmentKind::Variable : AssignmentKind::Signal;
        if (kind == AssignmentKind::Signal && !expectSymbol("<="))
            return std::nullopt;
        if (selector) {
            Assignment assignment;
            assignment.target = std::move(*target);
            assignment.indices = std::move(indices);
            assignment.selector = std::move(selector);
            do {
                auto value = parseExpression(0);
                if (!value || !expectWord("when")) return std::nullopt;
                std::vector<std::unique_ptr<Expression>> choices;
                const bool otherwise = acceptWord("others");
                if (!otherwise) do {
                    auto choice = parseIndex();
                    if (!choice) return std::nullopt;
                    choices.push_back(std::move(choice));
                } while (acceptSymbol("|"));
                assignment.alternatives.push_back(std::move(value));
                assignment.choices.push_back(std::move(choices));
                if (otherwise) break;
            } while (acceptSymbol(","));
            if (!expectSymbol(";")) return std::nullopt;
            assignment.span = join(start, lexed_.tokens[index_ - 1].span);
            return assignment;
        }
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
            auto operand = op.text == "+" || op.text == "-" ? parseExpression(5) : parsePrimary();
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
            auto parseElement = [&]() -> std::unique_ptr<Expression> {
                auto element = parseExpression(0);
                if (element && acceptSymbol("=>")) {
                    auto association = std::make_unique<Expression>();
                    association->kind = Expression::Kind::Association;
                    association->left = std::move(element);
                    association->right = parseExpression(0);
                    if (!association->right) return nullptr;
                    association->span = join(association->left->span, association->right->span);
                    return association;
                }
                return element;
            };
            auto nested = parseElement();
            if (nested && (symbol(",") || nested->kind == Expression::Kind::Association)) {
                auto aggregate = std::make_unique<Expression>();
                aggregate->kind = Expression::Kind::Aggregate;
                aggregate->elements.push_back(std::move(nested));
                while (acceptSymbol(",")) {
                    auto element = parseElement();
                    if (!element) return nullptr;
                    aggregate->elements.push_back(std::move(element));
                }
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
            while (symbol(".") || symbol("(") || symbol("'")) {
                if (acceptSymbol("'")) {
                    auto name = parseName();
                    if (!name) return nullptr;
                    auto attribute = std::make_unique<Expression>();
                    attribute->kind = Expression::Kind::Attribute;
                    attribute->text = name->spelling;
                    attribute->canonical = name->canonical;
                    attribute->span = join(expression->span, name->span);
                    attribute->left = std::move(expression);
                    expression = std::move(attribute);
                    continue;
                }
                if (acceptSymbol(".")) {
                    auto field = parseName();
                    if (!field) return nullptr;
                    auto selected = std::make_unique<Expression>();
                    selected->kind = Expression::Kind::Selected;
                    selected->text = field->spelling;
                    selected->canonical = field->canonical;
                    selected->span = join(expression->span, field->span);
                    selected->left = std::move(expression);
                    expression = std::move(selected);
                    continue;
                }
                advance();
                auto index = parseIndex();
                if (!index) return nullptr;
                auto associate = [&](std::unique_ptr<Expression> actual) -> std::unique_ptr<Expression> {
                    if (!acceptSymbol("=>")) return actual;
                    auto association = std::make_unique<Expression>();
                    association->kind = Expression::Kind::Association;
                    association->left = std::move(actual);
                    association->right = parseExpression(0);
                    if (!association->right) return nullptr;
                    association->span = join(association->left->span, association->right->span);
                    return association;
                };
                index = associate(std::move(index));
                if (!index) return nullptr;
                if (symbol(",") || index->kind == Expression::Kind::Association) {
                    auto call = std::make_unique<Expression>();
                    call->kind = Expression::Kind::Call;
                    call->left = std::move(expression);
                    call->elements.push_back(std::move(index));
                    while (acceptSymbol(",")) {
                        auto argument = parseExpression(0);
                        if (!argument) return nullptr;
                        argument = associate(std::move(argument));
                        if (!argument) return nullptr;
                        call->elements.push_back(std::move(argument));
                    }
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
    bool synthesis_ = false;
    ParseResult result_;
    std::size_t index_ = 0;
};

} // namespace

ParseResult Parser::parse(std::string_view source, bool synthesis) {
    return RecursiveParser(source, synthesis).run();
}

} // namespace vhdl
