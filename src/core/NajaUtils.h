// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __NAJA_UTILS_H_
#define __NAJA_UTILS_H_

#include <ostream>

namespace naja {

class NajaUtils {
  public:
    static void createBanner(std::ostream& stream, const std::string& title, const std::string& commentChar);
};

} // namespace naja

#endif // __NAJA_UTILS_H_