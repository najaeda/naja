// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <ostream>

namespace naja {

class NajaUtils {
  public:
    static void createBanner(std::ostream& stream, const std::string& title, const std::string& commentChar);
};

} // namespace naja

