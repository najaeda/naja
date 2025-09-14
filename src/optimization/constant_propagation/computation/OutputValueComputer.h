// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <DNL.h>
#include <unordered_map>
#include <set>

using namespace naja::DNL;
using namespace naja::NL;

namespace naja::NAJA_OPT {

class OutputValueComputer {
 public:
  enum Type { AND = 1, OR, XOR, NAND, NOR, XNOR, INV, BUF, HA, DFF, MUX, OAI };

  static unsigned computeOutputValue(DNLID instanceID, DNLFull* dnl, 
                                   const std::set<DNLID>& constants0, 
                                   const std::set<DNLID>& constants1);
  
  static unsigned computeOutputValueForConstantInstance(DNLID instanceID, DNLFull* dnl,
                                                      const std::unordered_map<NLID::DesignID, DNLID>& designObjectID2Type,
                                                      const std::set<DNLID>& constants0, 
                                                      const std::set<DNLID>& constants1);
  
  static unsigned computeOutputValueForPartiallyConstantInstance(DNLID instanceID, DNLFull* dnl,
                                                               const std::unordered_map<NLID::DesignID, DNLID>& designObjectID2Type,
                                                               const std::set<DNLID>& constants0, 
                                                               const std::set<DNLID>& constants1);
};

}  // namespace naja::NAJA_OPT