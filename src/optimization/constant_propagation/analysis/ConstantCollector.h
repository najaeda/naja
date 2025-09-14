// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <DNL.h>
#include <set>
#include <unordered_map>

using namespace naja::DNL;
using namespace naja::NL;

namespace naja::NAJA_OPT {

class ConstantCollector {
 public:
  enum Type { AND = 1, OR, XOR, NAND, NOR, XNOR, INV, BUF, HA, DFF, MUX, OAI };

  void initializeTypesID(DNLFull* dnl, std::unordered_map<NLID::DesignID, DNLID>& designObjectID2Type);
  void collectConstants(DNLFull* dnl, std::set<DNLID>& constants0, std::set<DNLID>& constants1, 
                       std::set<DNLID>& initialConstants0, std::set<DNLID>& initialConstants1);
};

}  // namespace naja::NAJA_OPT