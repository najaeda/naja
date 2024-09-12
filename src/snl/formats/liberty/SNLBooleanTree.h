
// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_BOOLEAN_TREE_H_
#define __SNL_BOOLEAN_TREE_H_

#include "SNLBitTerm.h"
#include "SNLTruthTable.h"

namespace naja { namespace SNL {

class SNLDesign;
class SNLBitTerm;

class SNLBooleanTreeNode {
  public:
    SNLBooleanTreeNode(const SNLBooleanTreeNode&) = delete;
    virtual bool getValue() const = 0;
    virtual ~SNLBooleanTreeNode() = default;
  protected:
    SNLBooleanTreeNode() = default;
};

class SNLBooleanTreeInputNode: public SNLBooleanTreeNode {
  public:
    enum class Type { INPUT, CONSTANT0, CONSTANT1 };
    SNLBooleanTreeInputNode(const SNLBitTerm* input):
      type_(Type::INPUT),
      term_(input)
    {}

    SNLBooleanTreeInputNode(Type type):
      type_(type),
      term_(nullptr)
    {}

    void setValue(bool value) { value_ = value; }

    bool getValue() const override {
      switch (type_) {
        case Type::INPUT:
          return value_;
        case Type::CONSTANT0:
          return false;
        case Type::CONSTANT1:
          return true;
      }
    }

    const SNLBitTerm* getTerm() const {
      return term_;
    }

  private:
    Type              type_     {Type::INPUT};
    const SNLBitTerm* term_     {nullptr};
    bool              value_    {false};
};

class SNLBooleanTreeFunctionNode: public SNLBooleanTreeNode {
  public:
    enum class Type { AND, OR, XOR, NOT, BUFFER };
    using Inputs = std::vector<SNLBooleanTreeNode*>;

    SNLBooleanTreeFunctionNode(Type type):
      type_(type)
    {}

    ~SNLBooleanTreeFunctionNode();

    void addInput(SNLBooleanTreeNode* input) {
      inputs_.push_back(input);
    }

    Type getType() const { return type_; }

    bool getValue() const override;

  private:
    Type    type_;
    Inputs  inputs_;
};

class SNLBooleanTree {
  public:
    using Inputs = std::map<const SNLBitTerm*, SNLBooleanTreeInputNode*, SNLBitTerm::PointerLess>;

    SNLBooleanTree() = default;
    ~SNLBooleanTree();

    SNLBooleanTreeInputNode* parseInput(
      const SNLDesign* primitive,
      const std::string& function,
      size_t& pos);
    void parse(const SNLDesign* primitive, const std::string& function);
    SNLBooleanTreeFunctionNode* getRoot() const { return root_; }
    const Inputs& getInputs() const { return inputs_; }
    SNLBooleanTreeInputNode* getInput(const SNLBitTerm* inputTerm) const;

    using Terms = std::vector<SNLBitTerm*>;
    SNLTruthTable getTruthTable(const Terms& terms);
  private:
    SNLBooleanTreeInputNode* getOrCreateInputNode(const SNLBitTerm* input);

    std::string                 function_ {};
    Inputs                      inputs_   {};
    SNLBooleanTreeFunctionNode* root_     {nullptr};  
};

}} // namespace SNL // namespace naja

#endif // __SNL_BOOLEAN_TREE_H_
