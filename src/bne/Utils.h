// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "DNL.h"
#include "SNLPath.h"

using namespace naja::DNL;
using namespace naja::SNL;

namespace naja::BNE {

class NetlistStatistics {
 public:
  NetlistStatistics(DNLFull& dnl) : dnl_(dnl) {}
  void process();
  const std::string& getReport() const { return report_; }

 private:
  DNLFull& dnl_;
  std::string report_;
};

SNLInstance *getInstanceForPath(const std::vector<SNLID::DesignObjectID> &pathToModel);

}  // namespace naja::BNE
