// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include <iostream>

#include "SNLUniverse.h"
#include "SNLCapnP.h"
#include "SNLUniverseSnippet.h"

using namespace naja::SNL;

int main(int argc, char* argv[]) {
  if (argc != 3) {
    exit(34);
  }
  std::string ipAddress = argv[1];
  int port = std::stoi(argv[2]);

  SNLUniverseSnippet::create();
  auto universe = SNLUniverse::get();
  assert(universe);
  auto db = universe->getDB(1);
  assert(db);

  std::cout << "Sending " << db->getString() << std::endl;
  SNLCapnP::send(db, ipAddress, port);
  return 0;
/*
  auto topIns1 = top->getInstance(SNLName("ins1"));
  std::cout << topIns1->getName().getString() << " instance terminals:" << std::endl;
  for (auto instTerm: topIns1->getInstTerms()) {
    std::cout << "  - " << instTerm->getTerm()->getName().getString() << std::endl;
  }

  auto topNet1 = top->getScalarNet(0); // net1 is an anonymous net at ID 0
  std::cout << topNet1->getString() << " components:" << std::endl;
  for (auto component: topNet1->getComponents()) {
    std::cout << "  - " << component->getString() << std::endl;
  }

*/
  
  return 0;
}
