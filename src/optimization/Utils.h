// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "DNL.h"

using namespace naja::DNL;
using namespace naja::SNL;

namespace naja::NAJA_OPT {
class Uniquifier {
 public:
  Uniquifier(std::vector<SNLID::DesignObjectID>& path, DNLID id)
      : path_(path), id_(id) {}
  void process();
  SNLInstance* replaceWithClone(SNLInstance* inst);
  std::vector<SNLInstance*>& getPathUniq() { return pathUniq_; }
  std::string getFullPath();

 private:
  std::vector<SNLID::DesignObjectID>& path_;
  std::vector<SNLInstance*> pathUniq_;
  DNLID id_ = DNLID_MAX;
};

class NetlistStatistics {
 public:
  NetlistStatistics(DNLFull& dnl) : dnl_(dnl) {}
  void process();
  const std::string& getReport() const { return report_; }

 private:
  DNLFull& dnl_;
  std::string report_;
};

}  // namespace naja::NAJA_OPT