// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_PYEDIT_H_
#define __SNL_PYEDIT_H_

#include <filesystem>

namespace naja { namespace NL {

class SNLPyEdit {
  public:
    static void edit(const std::filesystem::path& scriptPath);
};

}} // namespace NL // namespace naja

#endif // __SNL_PYEDIT_H_