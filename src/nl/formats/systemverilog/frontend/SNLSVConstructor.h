// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace naja::NL {

class NLLibrary;

class SNLSVConstructor {
  public:
    using Paths = std::vector<std::filesystem::path>;
    struct Config {
      bool blackboxDetection_ {true};  ///< If true, detect empty port-only modules as blackboxes.
    };

    Config config_ {};

    struct ConstructOptions {
      std::optional<std::filesystem::path> elaboratedASTJsonPath {};
      std::optional<std::filesystem::path> diagnosticsReportPath {};
      bool prettyPrintElaboratedASTJson {true};
      bool includeSourceInfoInElaboratedASTJson {true};
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

namespace detail {

struct SourceExcerptTestOptions {
    std::string sourceText;
    std::optional<size_t> startOffset {0};
    std::optional<size_t> endOffset {1};
    size_t maxLength {160};
    bool preserveRawBufferSize {false};
    bool useAlternateEndBuffer {false};
};

std::optional<std::string> testSVConstructorGetSourceExcerpt(
  const SourceExcerptTestOptions& options);

std::string testSVConstructorFormatReasonWithSourceExcerptNoRange(
  const std::string& reason);

}  // namespace detail

}  // namespace naja::NL
