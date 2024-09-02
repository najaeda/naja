// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "actions.h"
#include <spdlog/spdlog.h>
#include <iostream>
#include <set>
#include <stack>
#include <vector>
#include "SNLDesignModeling.h"
#include "SNLLibraryTruthTables.h"
#include "SNLScalarNet.h"
#include "Utils.h"
#include <ranges>
#include "SNLDesignTruthTable.h"
#include "SNLTruthTable.h"

using namespace naja::DNL;
using namespace naja::NAJA_OPT;
using namespace naja::SNL;

void changeDriverToLocal0(SNLInstTerm *term)
{
  term->setNet(nullptr);
  std::string name(std::string("logic0_naja_") +
                   term->getDesign()->getName().getString());
  auto netName = SNLName(name + "_net");
  SNLNet *assign0 = term->getDesign()->getNet(netName);
  if (nullptr == assign0)
  {
    assign0 = SNLScalarNet::create(term->getDesign(), netName);
  }
  assign0->setType(naja::SNL::SNLNet::Type::Supply0);
  term->setNet(assign0);
  SNLTruthTable tt(0, 0);
  // find primitives library
  if (term->getDB()->getPrimitiveLibraries().size() != 1)
  {
    // LCOV_EXCL_START
    throw SNLException("There should be only one primitive library");
    // LCOV_EXCL_STOP
  }
  auto primitives = *term->getDB()->getPrimitiveLibraries().begin();
  auto logic0 =
      SNLLibraryTruthTables::getDesignForTruthTable(primitives, tt).first;

  SNLInstance *logic0Inst = term->getDesign()->getInstance(SNLName(name));
  if (nullptr == logic0Inst)
  {
    if (logic0 == nullptr)
    {
      // LCOV_EXCL_START
      throw SNLException("No logic0 design found");
      // LCOV_EXCL_STOP
    }
    logic0Inst = SNLInstance::create(term->getDesign(), logic0, SNLName(name));
  }
  (*logic0Inst->getInstTerms().begin())->setNet(assign0);
}

void changeDriverToLocal1(SNLInstTerm *term)
{
  term->setNet(nullptr);
  std::string name(std::string("logic1_naja_") +
                   term->getDesign()->getName().getString());
  auto netName = SNLName(name + "_net");
  SNLNet *assign1 = term->getDesign()->getNet(netName);
  if (nullptr == assign1)
  {
    assign1 = SNLScalarNet::create(term->getDesign(), netName);
  }
  assign1->setType(naja::SNL::SNLNet::Type::Supply1);
  term->setNet(assign1);
  SNLTruthTable tt(0, 1);

  // find primitives library
  if (term->getDB()->getPrimitiveLibraries().size() != 1)
  {
    // LCOV_EXCL_START
    throw SNLException("There should be only one primitive library");
    // LCOV_EXCL_STOP
  }
  auto primitives = *term->getDB()->getPrimitiveLibraries().begin();
  auto logic1 =
      SNLLibraryTruthTables::getDesignForTruthTable(primitives, tt).first;
  SNLInstance *logic1Inst = term->getDesign()->getInstance(SNLName(name));
  if (nullptr == logic1Inst)
  {
    if (logic1 == nullptr)
    {
      // LCOV_EXCL_START
      throw SNLException("No logic1 design found");
      // LCOV_EXCL_STOP
    }
    logic1Inst = SNLInstance::create(term->getDesign(), logic1, SNLName(name));
  }
  (*logic1Inst->getInstTerms().begin())->setNet(assign1);
}

void changeDriverto0Top(SNLBitTerm *term)
{
  term->setNet(nullptr);
  std::string name(std::string("logic0_naja_") +
                   term->getDesign()->getName().getString());
  auto netName = SNLName(name + "_net");
  SNLNet *assign0 = term->getDesign()->getNet(netName);
  if (nullptr == assign0)
  {
    assign0 = SNLScalarNet::create(term->getDesign(), netName);
  }
  assign0->setType(naja::SNL::SNLNet::Type::Supply0);
  term->setNet(assign0);
  SNLTruthTable tt(0, 0);
  auto logic0 = SNLLibraryTruthTables::getDesignForTruthTable(
                    *(term->getDB()->getPrimitiveLibraries().begin()),
                    tt)
                    .first;
  SNLInstance *logic0Inst = term->getDesign()->getInstance(SNLName(name));
  if (nullptr == logic0Inst)
  {
    logic0Inst =
        SNLInstance::create(term->getDesign(), logic0, SNLName(name));
  }
  (*logic0Inst->getInstTerms().begin())->setNet(assign0);
}

void changeDriverto1Top(SNLBitTerm *term)
{
  term->setNet(nullptr);
  std::string name(std::string("logic1_naja_") +
                   term->getDesign()->getName().getString());
  auto netName = SNLName(name + "_net");
  SNLNet *assign1 = term->getDesign()->getNet(netName);
  if (nullptr == assign1)
  {
    assign1 = SNLScalarNet::create(term->getDesign(), netName);
  }
  assign1->setType(naja::SNL::SNLNet::Type::Supply1);
  term->setNet(assign1);
  SNLTruthTable tt(0, 1);
  auto logic1 = SNLLibraryTruthTables::getDesignForTruthTable(
                    *(term->getDB()->getPrimitiveLibraries().begin()),
                    tt)
                    .first;
  SNLInstance *logic1Inst = term->getDesign()->getInstance(SNLName(name));
  if (nullptr == logic1Inst)
  {
    logic1Inst =
        SNLInstance::create(term->getDesign(), logic1, SNLName(name));
  }
  (*logic1Inst->getInstTerms().begin())->setNet(assign1);
}

void DriveWithConstantAction::processOnContext(SNLDesign *design)
{
  if (value_ == 0)
  {
    if (context_.empty() && pathToDrive_ == (unsigned)(-1) && termToDrive_ == (unsigned)(-1))
    {
      assert(topTermToDrive_ != nullptr);
      changeDriverto0Top(topTermToDrive_);
    }
    else
    {
      changeDriverToLocal0(design->getInstance(pathToDrive_)->getInstTerm(termToDrive_));
    }
  }
  else if (value_ == 1)
  {
    if (context_.empty() && pathToDrive_ == (unsigned)(-1) && termToDrive_ == (unsigned)(-1))
    {
      assert(topTermToDrive_ != nullptr);
      changeDriverto1Top(topTermToDrive_);
    }
    else
    {
      changeDriverToLocal1(design->getInstance(pathToDrive_)->getInstTerm(termToDrive_));
    }
  }
  else
  {
    // LCOV_EXCL_START
    throw SNLException("Value should be 0 or 1");
    // LCOV_EXCL_STOP
  }
}

void DeleteAction::processOnContext(SNLDesign *design2)
{
  SNLDesign *top = SNLUniverse::get()->getTopDesign();
  SNLDesign *design = top;
  std::vector<SNLID::DesignObjectID> path =
      context_;
  path.push_back(toDelete_);
  // path.pop_back();
  std::string id("");
  std::vector<SNLID::DesignObjectID> pathIds;
  printf("delete ");
  while (!path.empty())
  {
    SNLID::DesignObjectID name = path.front();
    path.erase(path.begin());
    SNLInstance *inst = design->getInstance(name);
    assert(inst);
    design = inst->getModel();
    printf("%s ", inst->getName().getString().c_str());
    id += "_" + std::to_string(design->getID()) + "_" + std::to_string(inst->getID());
    pathIds.push_back(inst->getID());
  }
  naja::NAJA_OPT::Uniquifier uniquifier(pathIds, id, true);
  uniquifier.process();
  /*for (SNLInstTerm *term : uniquifier.getPathUniq().back()->getInstTerms())
  {
    auto net = term->getNet();
    term->setNet(nullptr);
    if (net != nullptr)
    {
      if (net->getInstTerms().size() + net->getBitTerms().size() == 0)
      {
        net->destroy();
      }
    }
  }*/
  printf("\n");
  uniquifier.getPathUniq().back()->destroy();
  // design->getInstance(pathToDelete_.back())->destroy();
}