// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include <iostream>

#include "SNLDB.h"
#include "SNLCapnP.h"

using namespace naja::SNL;

namespace {

void displayDesign(const SNLDesign* design) {
  std::cout << design->getDescription() << std::endl;
  for (auto term: design->getTerms()) {
    std::cout << term->getDescription() << std::endl;
  }
  for (auto net: design->getNets()) {
    std::cout << net->getDescription() << std::endl;
  }
  for (auto instance: design->getInstances()) {
    std::cout << instance->getDescription() << std::endl;
  }
}

void displayLibrary(const SNLLibrary* lib) {
  std::cout << lib->getDescription() << std::endl;
  for (auto design: lib->getDesigns()) {
    displayDesign(design);
  }
}

void displayDB(const SNLDB* db) {
  std::cout << db->getDescription() << std::endl;
  for (auto lib: db->getLibraries()) {
    displayLibrary(lib);
  }
}

}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    exit(34);
  }
  int port = std::stoi(argv[1]);

  SNLDB* db = SNLCapnP::receive(port);
  if (db) {
    std::cout << "Received " << db->getString() << std::endl; 
    displayDB(db);
  } else {
    std::cout << "No DB received" << std::endl; 
  }
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
