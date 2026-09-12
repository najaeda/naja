// SPDX-FileCopyrightText: Michael Popoloski
// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: MIT

#include "slang/numeric/SVInt.h"

namespace slang {

bool NajaPyslang_literalBaseFromChar(char base, LiteralBase& result) {
  switch (base) {
    case 'd':
    case 'D':
      result = LiteralBase::Decimal;
      return true;
    case 'b':
    case 'B':
      result = LiteralBase::Binary;
      return true;
    case 'o':
    case 'O':
      result = LiteralBase::Octal;
      return true;
    case 'h':
    case 'H':
      result = LiteralBase::Hex;
      return true;
    default:
      return false;
  }
}

}  // namespace slang
