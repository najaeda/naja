#include "SchematicSNL.h"

#include "SNLDesign.h"
using namespace naja::SNL;

#include "Schematic.h"

Schematic* SchematicSNL::constructFromSNLDesign(const naja::SNL::SNLDesign* design) {
  Schematic* schematic = new Schematic();
  for (auto instance: design->getInstances()) {
    auto model = instance->getModel();
    auto node = new Node(schematic);
    for (auto term: model->getTerms()) {
      auto pin = new Pin(node, Pin::Direction::Input);
    }
  }
  return schematic;
}