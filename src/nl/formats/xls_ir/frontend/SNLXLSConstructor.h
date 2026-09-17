// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace naja::NL {

class NLLibrary;
class SNLDesign;

/** Width-explicit parameter received from the validated XLS bridge. */
struct SNLXLSIRParameter {
  std::string name{};
  size_t width{0};
};

/**
 * Width-explicit combinational node received from the validated XLS bridge.
 *
 * Operation names use XLS spelling. The initial supported set is ``add``,
 * ``sub``, and two-case ``sel``. Operands refer to parameter or node names.
 */
struct SNLXLSIRNode {
  int64_t id{0};
  std::string name{};
  std::string op{};
  size_t width{0};
  std::vector<std::string> operands{};
  std::string source{};
};

/** A bits-only combinational XLS function after parsing and verification. */
struct SNLXLSIRFunction {
  std::string name{};
  std::vector<SNLXLSIRParameter> parameters{};
  std::vector<SNLXLSIRNode> nodes{};
  std::string result{};
  std::string outputName{"result"};
};

/** Lower validated bits-only XLS function records into SNL designs. */
class SNLXLSConstructor {
  public:
    SNLXLSConstructor() = delete;
    SNLXLSConstructor(const SNLXLSConstructor&) = delete;
    explicit SNLXLSConstructor(NLLibrary* library);

    SNLDesign* construct(const SNLXLSIRFunction& function);

  private:
    NLLibrary* library_{nullptr};
};

}  // namespace naja::NL
