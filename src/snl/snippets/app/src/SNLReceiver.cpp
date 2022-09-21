#include <iostream>
#include "SNLCapnP.h"

using namespace naja::SNL;

int main(int argc, char* argv[]) {
  if (argc != 2) {
    exit(34);
  }
  int port = std::stoi(argv[1]);

  SNLDB* db = SNLCapnP::receiveInterface(port);
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