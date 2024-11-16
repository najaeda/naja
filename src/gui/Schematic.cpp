#include "Schematic.h"

Pin::Direction::Direction(const DirectionEnum& dirEnum):
  dirEnum_(dirEnum) 
{}

//LCOV_EXCL_START
std::string Pin::Direction::getString() const {
  switch (dirEnum_) {
    case Direction::Input: return "Input";
    case Direction::Output: return "Output";
  }
  return "Unknown";
}
//LCOV_EXCL_STOP

Pin::Pin(Node* node, const Direction& direction):
  node_(node),
  direction_(direction)
{}

Node::Node(Schematic* schematic, const std::string& name):
  name_(name) {
  schematic->addNode(this);
}

void Node::layout() {
  size_t numInputs = inputs_.size();
  size_t numOutputs = outputs_.size();
  size_t maxPins = std::max(numInputs, numOutputs);

  float width = 100.0f;
  float height = 20.0f * maxPins + 20.0f;

  for (size_t i=0; i<numInputs; ++i) {
    auto pin = inputs_[i];
    pin->setPosition(Position{0, 20.0f + 20.0f * i});
  }

  for (size_t i=0; i<numOutputs; ++i) {
    auto pin = outputs_[i];
    pin->setPosition(Position{width, 20.0f + 20.0f * i});
  }
}

void Node::addPin(Pin* pin) {
  if (pin->getDirection() == Pin::Direction::Input) {
    inputs_.push_back(pin);
  } else {
    outputs_.push_back(pin);
  }
}

void Schematic::addNode(Node* node) {
  nodes_.push_back(node);
}