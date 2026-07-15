// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace slang::ast {
class Compilation;
class Symbol;
}

namespace slang::driver {
class Driver;
}

namespace slang::syntax {
class SyntaxTree;
}

namespace naja::NL {

class NLDB;
class NLLibrary;
class NLObject;

class SNLSVLiveASTLink {
  public:
    using Objects = std::vector<NLObject*>;
    SNLSVLiveASTLink() = default;
    ~SNLSVLiveASTLink();

    SNLSVLiveASTLink(const SNLSVLiveASTLink&) = delete;
    SNLSVLiveASTLink& operator=(const SNLSVLiveASTLink&) = delete;

    void bind(NLObject* object, const slang::ast::Symbol& symbol);
    const slang::ast::Symbol* getSymbol(const NLObject* object) const;
    const Objects& getObjects(const slang::ast::Symbol* symbol) const;
    bool hasSymbol(const slang::ast::Symbol* symbol) const;
    slang::ast::Compilation* getCompilation() const { return compilation_.get(); }

    std::unique_ptr<slang::driver::Driver> driver_ {};
    std::unique_ptr<slang::ast::Compilation> compilation_ {};
    std::vector<std::shared_ptr<slang::syntax::SyntaxTree>> syntaxTrees_ {};

  private:
    std::unordered_map<const NLObject*, const slang::ast::Symbol*> symbolByObject_ {};
    std::unordered_map<const slang::ast::Symbol*, Objects> objectsBySymbol_ {};
};

class SNLSVLiveASTLinkRegistry {
  public:
    static void clearAll();
    static void clear(NLDB* db);
    static void store(NLDB* db, std::unique_ptr<SNLSVLiveASTLink> link);
    static const SNLSVLiveASTLink* get(NLDB* db);
    static const SNLSVLiveASTLink* getLatest();
    static const SNLSVLiveASTLink* findForObject(const NLObject* object);
    static const SNLSVLiveASTLink* findForSymbol(const slang::ast::Symbol* symbol);
};

class SNLSVConstructor {
  public:
    using Paths = std::vector<std::filesystem::path>;
    struct ConstructOptions {
      /// If true, detect empty port-only modules as blackboxes.
      bool blackboxDetection {true};
      std::optional<std::filesystem::path> elaboratedASTJsonPath {};
      /// Incremental diagnostics report. A report is always written; callers may
      /// override this default path but an empty path is invalid.
      std::filesystem::path diagnosticsReportPath {"naja_sv_diagnostics.log"};
      bool prettyPrintElaboratedASTJson {true};
      bool includeSourceInfoInElaboratedASTJson {true};
      /// Request retention of live frontend AST to SNL object links after load.
      bool keepASTLink {false};
      /// SystemVerilog preprocessor defines (passed as -D<name>[=<value>] to the driver).
      std::vector<std::string> preprocessorDefines {};
      /// SystemVerilog frontend warning names to suppress.
      std::vector<std::string> suppressWarnings {};
      /// Treat instantiations of unresolved (unknown) modules as auto-blackboxes
      /// instead of failing the load. Slang's UnknownModule/UnknownPrimitive
      /// errors are demoted, and each unknown instance gets an AutoBlackBox model
      /// whose terms are inferred from the instantiation's port connections
      /// (direction defaults to InOut, width to the connected expression width).
      bool blackboxUnknownModules {false};
    };

    SNLSVConstructor() = delete;
    SNLSVConstructor(const SNLSVConstructor&) = delete;
    SNLSVConstructor(NLLibrary* library);

    void construct(const Paths& paths);
    void construct(const std::filesystem::path& path);
    void construct(const Paths& paths, const ConstructOptions& options);
    void construct(const std::filesystem::path& path, const ConstructOptions& options);

  private:
    NLLibrary* library_ {nullptr};
};

}  // namespace naja::NL
