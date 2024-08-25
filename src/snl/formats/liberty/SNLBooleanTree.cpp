// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLBooleanTree.h"

#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"

namespace {

struct Token {
  char                            type_;
  naja::SNL::SNLBooleanTreeNode*  node_ {nullptr};
  
  Token(char t) : type_(t) {}
  Token(char t, naja::SNL::SNLBooleanTreeNode* n) : type_(t), node_(n) {}
};

using Stack = std::vector<Token>;

using namespace naja::SNL;

bool reduce(
  const SNLDesign* primitive,
  Stack& stack,
  const Token& nextToken) {

  int top = int(stack.size())-1;

  if (0 <= top-1 and stack[top].type_ == 0 and stack[top-1].type_ == '!') {
    auto notNode = new SNLBooleanTreeFunctionNode(SNLBooleanTreeFunctionNode::Type::NOT);
    notNode->addInput(stack[top].node_);
    stack.pop_back();
    stack.pop_back();
    stack.push_back(Token(0, notNode));
    return true;
  }

  if (0 <= top-1 and stack[top].type_ == '\'' and stack[top-1].type_ == 0) {
    auto notNode = new SNLBooleanTreeFunctionNode(SNLBooleanTreeFunctionNode::Type::NOT);
    notNode->addInput(stack[top-1].node_);
    stack.pop_back(); 
    stack.pop_back();
    stack.push_back(Token(0, notNode));
    return true;
  }

  if (0 <= top and stack[top].type_ == 0) {
    if (nextToken.type_ == '\'')
      return false;
    stack[top].type_ = 1;
    return true;
  }

  if (0 <= top-2 and stack[top-2].type_ == 1 and stack[top-1].type_ == '^' and stack[top].type_ == 1) {
    auto xorNode = new SNLBooleanTreeFunctionNode(SNLBooleanTreeFunctionNode::Type::XOR);
    xorNode->addInput(stack[top-2].node_);
    xorNode->addInput(stack[top].node_);
    stack.pop_back();
    stack.pop_back();
    stack.pop_back();
    stack.push_back(Token(1, xorNode));
    return true;
  }




  return false;
}

} // namespace

namespace naja { namespace SNL {

SNLBooleanTreeInputNode* SNLBooleanTree::getOrCreateInputNode(const SNLBitTerm* input) {
  auto it = inputs_.find(input);
  if (it != inputs_.end()) {
    return it->second;
  }
  auto inputNode = new SNLBooleanTreeInputNode(input);
  inputs_[input] = inputNode;
  return inputNode;
}

SNLBooleanTreeInputNode* SNLBooleanTree::parseInput(
  const SNLDesign* primitive,
  const std::string& function,
  size_t& pos) {
    char car = function[pos];
    int idLen = 0;
    while (('a' <= car and car <= 'z')
      or ('A' <= car and car <= 'Z')
      or ('0' <= car and car <= '9')
      or car == '.'
      or car == '_'
      or car == '['
      or car == ']') {
      ++idLen;
      car = function[pos + idLen];
    }

    if (idLen == 0) {
      throw std::runtime_error("Expected identifier at `" + function.substr(pos) + "'.");
    }

 //  if (id_len == 1 && (*expr == '0' || *expr == '1'))
 //   return *(expr++) == '0' ? RTLIL::State::S0 : RTLIL::State::S1;

    auto inputName = function.substr(pos, idLen);
    SNLBitTerm* input = nullptr;

    //is it a bus ?
    auto start = inputName.find('[');
    if (start != std::string::npos) {
      auto stop = inputName.find(']');
      if (stop == std::string::npos) {
        throw std::runtime_error("Expected `]' at `" + function.substr(pos) + "'.");
      }
      auto busName = inputName.substr(0, start);
      auto busIndex = inputName.substr(start + 1, stop - start - 1);
      auto bus = primitive->getBusTerm(SNLName(busName));
      if (bus == nullptr) {
        throw std::runtime_error("Bus `" + busName + "' not found.");
      }
      auto index = std::stoi(busIndex);
      input = bus->getBit(index);
      if (input == nullptr) {
        throw std::runtime_error("Bit `" + busIndex + "' not found in bus `" + busName + "'.");
      }
    } else {
      input = primitive->getScalarTerm(SNLName(inputName));
      if (input == nullptr) {
        throw std::runtime_error("Scalar `" + inputName + "' not found.");
      }
    }

    auto inputNode = getOrCreateInputNode(input);

    pos += idLen;
    return inputNode;
}

SNLBooleanTree* SNLBooleanTree::parse(const SNLDesign* primitive, const std::string& function) {
  SNLBooleanTree* tree = new SNLBooleanTree();
  size_t pos = 0;
  Stack stack;
  while (pos < function.size()) {
    const char& car = function[pos];
    if (std::isspace(car) or car == '"') {
      pos++;
      continue;
    }
    Token nextToken(0);
    switch (car) {
      case '(':
      case ')':
      case '\'':
      case '!':
      case '^':
      case '*':
      case '+':
      case '|':
      case '&':
        nextToken = Token(car);
        ++pos;
        break;
      default:
        {
          auto input = tree->parseInput(primitive, function, pos);
          nextToken = Token('i', input);
        }
        break;
    }
    while (reduce(primitive, stack, nextToken)) {}
    stack.push_back(nextToken);
  }
  return tree;
}

bool SNLBooleanTreeFunctionNode::getValue() const {
  switch (type_) {
    case Type::AND:
      return std::all_of(inputs_.begin(), inputs_.end(), [](auto input) { return input->getValue(); });
    case Type::OR:
      return std::any_of(inputs_.begin(), inputs_.end(), [](auto input) { return input->getValue(); });
    case Type::XOR:
      return std::count_if(inputs_.begin(), inputs_.end(), [](auto input) { return input->getValue(); }) % 2;
    case Type::NOT:
      if (inputs_.size() != 1) {
        throw std::runtime_error("NOT node must have exactly one input");
      }
      return not inputs_[0]->getValue();
  }
}



}} // namespace SNL // namespace naja