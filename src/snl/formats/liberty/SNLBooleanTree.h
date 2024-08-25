
// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_LIBERTY_CONSTRUCTOR_H_
#define __SNL_LIBERTY_CONSTRUCTOR_H_

#include <string>
#include <vector>

namespace naja { namespace SNL {

class SNLDesign;
class SNLBitTerm;

class SNLBooleanTreeNode {
  public:
    SNLBooleanTreeNode(const SNLBooleanTreeNode&) = delete;
    virtual bool getValue() const = 0;
  protected:
    SNLBooleanTreeNode() = default;

};

class SNLBooleanTreeInputNode: public SNLBooleanTreeNode {
  public:
    SNLBooleanTreeInputNode(const SNLBitTerm* input):
      term_(input)
    {}

    void setValue(bool value) { value_ = value; }

    bool getValue() const override {
      return value_;
    }

  private:
    const SNLBitTerm* term_     {nullptr};
    bool              value_    {false};
};

class SNLBooleanTreeFunctionNode: public SNLBooleanTreeNode {
  public:
    enum class Type { AND, OR, XOR, NOT };
    using Inputs = std::vector<SNLBooleanTreeNode*>;

    SNLBooleanTreeFunctionNode(Type type):
      type_(type)
    {}

    void addInput(SNLBooleanTreeNode* input) {
      inputs_.push_back(input);
    }

    bool getValue() const override;

  private:
    Type    type_;
    Inputs  inputs_;
};

class SNLBooleanTree {
  public:
    using Inputs = std::map<const SNLBitTerm*, SNLBooleanTreeInputNode*, SNLBitTerm::PointerLess>;

    SNLBooleanTreeInputNode* parseInput(
      const SNLDesign* primitive,
      const std::string& function,
      size_t& pos);
    static void parse(const SNLDesign* primitive, const std::string& function);
  private:
    SNLBooleanTreeInputNode* getOrCreateInputNode(const SNLBitTerm* input);

    Inputs                      inputs_;
    SNLBooleanTreeFunctionNode* root_;
};

}} // namespace SNL // namespace naja

#endif // __SNL_LIBERTY_FUNCTION_GRAPH_H_