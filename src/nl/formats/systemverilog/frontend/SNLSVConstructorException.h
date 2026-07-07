// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "NLException.h"

namespace naja::NL {

struct SNLSVConstructorException: public NLException {
  public:
    SNLSVConstructorException() = delete;
    SNLSVConstructorException(const SNLSVConstructorException&) = default;

    SNLSVConstructorException(const std::string& reason):
      NLException(reason)
    {}
};

struct SNLSVDiagnostic {
  std::string severity;
  std::string file;
  size_t line {0};
  size_t column {0};
  std::string message;
};

struct SNLSVUnsupportedElement {
  std::string message;
  std::string code;
};

struct SNLSVSyntaxError: public SNLSVConstructorException {
  SNLSVSyntaxError() = delete;
  SNLSVSyntaxError(const SNLSVSyntaxError&) = default;

  SNLSVSyntaxError(
    const std::string& reason,
    std::vector<SNLSVDiagnostic> diagnostics):
    SNLSVConstructorException(reason),
    diagnostics_(std::move(diagnostics))
  {}

  const std::vector<SNLSVDiagnostic>& getDiagnostics() const {
    return diagnostics_;
  }

  private:
    std::vector<SNLSVDiagnostic> diagnostics_;
};

struct SNLSVUnsupportedConstructError: public SNLSVConstructorException {
  SNLSVUnsupportedConstructError() = delete;
  SNLSVUnsupportedConstructError(const SNLSVUnsupportedConstructError&) = default;

  SNLSVUnsupportedConstructError(
    const std::string& reason,
    std::vector<SNLSVUnsupportedElement> unsupportedElements):
    SNLSVConstructorException(reason),
    unsupportedElements_(std::move(unsupportedElements))
  {}

  const std::vector<SNLSVUnsupportedElement>& getUnsupportedElements() const {
    return unsupportedElements_;
  }

  private:
    std::vector<SNLSVUnsupportedElement> unsupportedElements_;
};

struct SNLSVInternalError: public SNLSVConstructorException {
  using SNLSVConstructorException::SNLSVConstructorException;
};

}  // namespace naja::NL
