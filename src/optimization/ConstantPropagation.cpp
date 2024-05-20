// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0
#include "ConstantPropagation.h"
#include <spdlog/spdlog.h>
#include <iostream>
#include <set>
#include <stack>
#include <vector>
#include "SNLScalarNet.h"
#include "Utils.h"

using namespace naja::DNL;
using namespace naja::NAJA_OPT;

//#define DEBUG_PRINTS

void ConstantPropagation::initializeTypesID() {
  for (DNLID leaf : dnl_->getLeaves()) {
    DNLInstanceFull instance = dnl_->getDNLInstanceFromID(leaf);
    std::string name = instance.getSNLModel()->getName().getString();
    if (name.find("NAND") != std::string::npos) {
      #ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("%s -> NAND\n", name.c_str());
      // LCOV_EXCL_STOP
      #endif
      designObjectID2Type_[instance.getSNLInstance()->getModel()->getID()] =
          Type::NAND;
    } else if (name.find("NOR") != std::string::npos) {
      #ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("%s -> NOR\n", name.c_str());
      // LCOV_EXCL_STOP
      #endif
      designObjectID2Type_[instance.getSNLInstance()->getModel()->getID()] =
          Type::NOR;
    } else if (name.find("XNOR") != std::string::npos) {
      #ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("%s -> XNOR\n", name.c_str());
      // LCOV_EXCL_STOP
      #endif
      designObjectID2Type_[instance.getSNLInstance()->getModel()->getID()] =
          Type::XNOR;
    } else if (name.find("XOR") != std::string::npos) {
      #ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("%s -> XOR\n", name.c_str());
      // LCOV_EXCL_STOP
      #endif
      designObjectID2Type_[instance.getSNLInstance()->getModel()->getID()] =
          Type::XOR;
    } else if (name.find("AND") != std::string::npos) {
      #ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("%s -> AND\n", name.c_str());
      // LCOV_EXCL_STOP
      #endif
      designObjectID2Type_[instance.getSNLInstance()->getModel()->getID()] =
          Type::AND;
    } else if (name.find("OR") != std::string::npos) {
      #ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("%s -> OR\n", name.c_str());
      // LCOV_EXCL_STOP
      #endif
      designObjectID2Type_[instance.getSNLInstance()->getModel()->getID()] =
          Type::OR;
    } else if (name.find("INV") != std::string::npos) {
      #ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("%s -> INV\n", name.c_str());
      // LCOV_EXCL_STOP
      #endif
      designObjectID2Type_[instance.getSNLInstance()->getModel()->getID()] =
          Type::INV;
    } else if (name.find("BUF") != std::string::npos) {
      #ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("%s -> BUF\n", name.c_str());
      // LCOV_EXCL_STOP
      #endif
      designObjectID2Type_[instance.getSNLInstance()->getModel()->getID()] =
          Type::BUF;
    } else if (name.find("HA") != std::string::npos) {
      #ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("%s -> HA\n", name.c_str());
      // LCOV_EXCL_STOP
      #endif
      designObjectID2Type_[instance.getSNLInstance()->getModel()->getID()] =
          Type::HA;
    } else if (name.find("DFF") != std::string::npos) {
      #ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("%s -> DFF\n", name.c_str());
      // LCOV_EXCL_STOP
      #endif
      designObjectID2Type_[instance.getSNLInstance()->getModel()->getID()] =
          Type::DFF;
    }
  }
}

void ConstantPropagation::collectConstants() {
  for (DNLID leaf : dnl_->getLeaves()) {
    DNLInstanceFull instance = dnl_->getDNLInstanceFromID(leaf);
    std::string name = instance.getSNLModel()->getName().getString();
    //printf("name: %s\n", name.c_str());
    if (name.find("LOGIC0") != std::string::npos) {
      for (DNLID termId = instance.getTermIndexes().first;
           termId <= instance.getTermIndexes().second; termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (term.getSnlBitTerm()->getDirection() ==
            SNLBitTerm::Direction::Output) {
          constants0_.insert(term.getIsoID());
        }
      }
    } else if (name.find("LOGIC1") != std::string::npos) {
      for (DNLID termId = instance.getTermIndexes().first;
           termId <= instance.getTermIndexes().second; termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (term.getSnlBitTerm()->getDirection() ==
            SNLBitTerm::Direction::Output) {
          constants1_.insert(term.getIsoID());
        }
      }
    }
  }
  constants0_.insert(dnl_->getDNLIsoDB().getConstant0Isos().begin(),
                     dnl_->getDNLIsoDB().getConstant0Isos().end());
  constants1_.insert(dnl_->getDNLIsoDB().getConstant1Isos().begin(),
                     dnl_->getDNLIsoDB().getConstant1Isos().end());
}

void ConstantPropagation::performConstantPropagationAnalysis() {
  std::set<DNLID> constants;
  constants.insert(constants0_.begin(), constants0_.end());
  constants.insert(constants1_.begin(), constants1_.end());
  #ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf("Constant Propagation : Number of constants before: %lu\n", constants.size());
  size_t loop = 0;
  // LCOV_EXCL_STOP
  #endif
  while (!constants.empty()) {
    #ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf("loop: %lu\n", loop);
    // LCOV_EXCL_STOP
    #endif
    loop++;
    std::set<DNLID> constantsNew;
    for (DNLID constant : constants) {
      DNLIso iso = dnl_->getDNLIsoDB().getIsoFromIsoIDconst(constant);
      for (DNLID readerTerm : iso.getReaders()) {
        DNLTerminalFull reader = dnl_->getDNLTerminalFromID(readerTerm);
        #ifdef DEBUG_PRINTS
        // LCOV_EXCL_START
        if (!reader.getDNLInstance().isTop()) {
          printf("%lu reader: %s\n", constant, reader.getDNLInstance().getSNLInstance()->getName().getString().c_str());
        } else {
          printf("%lu reader: %s\n", constant, dnl_->getTopDesign()->getName().getString().c_str());
        }
        // LCOV_EXCL_STOP
        #endif
        if (reader.getDNLInstance().isTop()) {
          continue;
        }
        bool isConst = true;
        std::vector<DNLTerminalFull> output;
        DNLTerminalFull q;
        for (DNLID termId = reader.getDNLInstance().getTermIndexes().first;
             termId <= reader.getDNLInstance().getTermIndexes().second;
             termId++) {
          const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
          if (term.getSnlBitTerm()->getDirection() ==
              SNLBitTerm::Direction::Output) {
            #ifdef DEBUG_PRINTS
            // LCOV_EXCL_START
                printf("output: %s\n", term.getSnlBitTerm()->getName().getString().c_str());
            // LCOV_EXCL_STOP
            #endif
            output.push_back(term);
            if (term.getSnlBitTerm()->getName().getString() == std::string("Q")) {
              q = term;
            }
            continue;
          }
          if (constants0_.find(term.getIsoID()) == constants0_.end() &&
              constants1_.find(term.getIsoID()) == constants1_.end()) {
            isConst = false;
          }
        }
        if (output.size() == 1 || !q.isNull()) {
          if (isConst) {
            partialConstantInstances_.erase(reader.getDNLInstance().getID());
            // Analyze the contants in ouptus and propagate them
            size_t newConst = computeOutputValueForConstantInstance(
                    reader.getDNLInstance().getID());
            if ( newConst == 1) {
              constantsNew.insert(output[0].getIsoID());
              constants1_.insert(output[0].getIsoID());
            } else if (newConst == 0) {
              constantsNew.insert(output[0].getIsoID());
              constants0_.insert(output[0].getIsoID());
            }
          } else {
            size_t newConst = computeOutputValueForPartiallyConstantInstance(
                reader.getDNLInstance().getID());
            if ( newConst == 1) {
              #ifdef DEBUG_PRINTS
              // LCOV_EXCL_START
              printf("%lu\n", output[0].getIsoID());
              // LCOV_EXCL_STOP
              #endif
              DNLID iso = DNLID_MAX;
              if (q.isNull()) {
                iso = output[0].getIsoID();
              } else {
                iso = q.getIsoID();
              }
              constantsNew.insert(iso);
              constants1_.insert(iso);
            } else if (newConst == 0) {
              DNLID iso = DNLID_MAX;
              if (q.isNull()) {
                iso = output[0].getIsoID();
              } else {
                iso = q.getIsoID();
              }
              constantsNew.insert(iso);
              constants0_.insert(iso);
            } else {
              partialConstantInstances_.insert(reader.getDNLInstance().getID());
            }
          }
        }
      }
    }
    constants = constantsNew;
  }
  #ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf("Constant Propagation : Number of constants after: %lu\n", constants0_.size() + constants1_.size());
  // LCOV_EXCL_STOP
  #endif
}

unsigned ConstantPropagation::computeOutputValueForConstantInstance(
    DNLID instanceID) {
  DNLInstanceFull instance = dnl_->getDNLInstanceFromID(instanceID);
  #ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf("f instance: %s\n", instance.getSNLInstance()->getName().getString().c_str());
  // LCOV_EXCL_STOP
  #endif
  switch (
      designObjectID2Type_[instance.getSNLInstance()->getModel()->getID()]) {
    case Type::AND: {
      for (DNLID termId = instance.getTermIndexes().first;
           termId <= instance.getTermIndexes().second; termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (constants0_.find(term.getIsoID()) != constants0_.end()) {
          #ifdef DEBUG_PRINTS
          // LCOV_EXCL_START
          printf("f AND 0\n");
          // LCOV_EXCL_STOP
          #endif
          return 0;
        }
      }
      #ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("f AND 1\n");
      // LCOV_EXCL_STOP
      #endif
      return 1;
    }
    case Type::OR: {
      for (DNLID termId = instance.getTermIndexes().first;
           termId <= instance.getTermIndexes().second; termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (constants1_.find(term.getIsoID()) != constants1_.end()) {
          #ifdef DEBUG_PRINTS
          // LCOV_EXCL_START
          printf("f OR 1\n");
          // LCOV_EXCL_STOP
          #endif
          return 1;
        }
      }
      #ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("f OR 0\n");
      // LCOV_EXCL_STOP
      #endif
      return 0;
    }
    case Type::XOR: {
      unsigned count = 0;
      for (DNLID termId = instance.getTermIndexes().first;
           termId <= instance.getTermIndexes().second; termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (constants1_.find(term.getIsoID()) != constants1_.end()) {
          count++;
        }
      }
      #ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("f XOR %d\n", count % 2);
      // LCOV_EXCL_STOP
      #endif
      return count % 2;
    }
    case Type::NAND: {
      for (DNLID termId = instance.getTermIndexes().first;
           termId <= instance.getTermIndexes().second; termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (constants0_.find(term.getIsoID()) != constants0_.end()) {
          #ifdef DEBUG_PRINTS
          // LCOV_EXCL_START
          printf("f NAND 1\n");
          // LCOV_EXCL_STOP
          #endif
          return 1;
        }
      }
      #ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("f NAND 0\n");
      // LCOV_EXCL_STOP
      #endif
      return 0;
    }
    case Type::NOR: {
      for (DNLID termId = instance.getTermIndexes().first;
           termId <= instance.getTermIndexes().second; termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (constants1_.find(term.getIsoID()) != constants1_.end()) {
          #ifdef DEBUG_PRINTS
          // LCOV_EXCL_START
          printf("f NOR 0\n");
          // LCOV_EXCL_STOP
          #endif
          return 0;
        }
      }
      #ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("f NOR 1\n");
      // LCOV_EXCL_STOP
      #endif
      return 1;
    }
    case Type::XNOR: {
      unsigned count = 0;
      for (DNLID termId = instance.getTermIndexes().first;
           termId <= instance.getTermIndexes().second; termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (constants1_.find(term.getIsoID()) != constants1_.end()) {
          count++;
        }
      }
      #ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("f XNOR %d\n", (count % 2) == 0);
      // LCOV_EXCL_STOP
      #endif
      return (count % 2) == 0;
    }
    case Type::INV: {
      DNLID termId = instance.getTermIndexes().first;
      const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
      if (constants1_.find(term.getIsoID()) != constants1_.end()) {
        #ifdef DEBUG_PRINTS
        // LCOV_EXCL_START
        printf("f INV 0\n");
        // LCOV_EXCL_STOP
        #endif
        return 0;
      } else {
        #ifdef DEBUG_PRINTS
        // LCOV_EXCL_START
        printf("f INV 1\n");
        // LCOV_EXCL_STOP
        #endif
        return 1;
      }
    } 
    case Type::BUF: {
      DNLID termId = instance.getTermIndexes().first;
      const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
      if (constants1_.find(term.getIsoID()) != constants1_.end()) {
        #ifdef DEBUG_PRINTS
        // LCOV_EXCL_START
        printf("f BUF 1\n");
        // LCOV_EXCL_STOP
        #endif
        return 1;
      } else {
        #ifdef DEBUG_PRINTS
        // LCOV_EXCL_START
        printf("f BUF 0\n");
        // LCOV_EXCL_STOP
        #endif
        return 0;
      }
    }
    case Type::DFF: {
      unsigned d = (unsigned) -1;
      for (DNLID termId = instance.getTermIndexes().first;
           termId <= instance.getTermIndexes().second; termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (term.getSnlBitTerm()->getName().getString() == std::string("D")) {
          if (constants1_.find(term.getIsoID()) != constants1_.end()) {
            #ifdef DEBUG_PRINTS
            // LCOV_EXCL_START
            printf("f DFF 1\n");
            // LCOV_EXCL_STOP
            #endif
            d = 1;
          } else if (constants0_.find(term.getIsoID()) != constants0_.end()) {
            #ifdef DEBUG_PRINTS
            // LCOV_EXCL_START
            printf("f DFF 0\n");
            // LCOV_EXCL_STOP
            #endif
            d = 0;
          }
          break;
        }
      }
      return d;
    }
    default:
      break;
  }
  return (unsigned)-1;
}

unsigned ConstantPropagation::computeOutputValueForPartiallyConstantInstance(
    DNLID instanceID) {
  DNLInstanceFull instance = dnl_->getDNLInstanceFromID(instanceID);
  #ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf("p instance: %s\n", instance.getSNLInstance()->getName().getString().c_str());
  // LCOV_EXCL_STOP
  #endif
  switch (
      designObjectID2Type_[instance.getSNLInstance()->getModel()->getID()]) {
    case Type::AND: {
      for (DNLID termId = instance.getTermIndexes().first;
           termId <= instance.getTermIndexes().second; termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (constants0_.find(term.getIsoID()) != constants0_.end()) {
          #ifdef DEBUG_PRINTS
          // LCOV_EXCL_START
          printf("p AND 0\n");
          // LCOV_EXCL_STOP
          #endif
          return 0;
        }
      }
      #ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("p AND nan\n");
      // LCOV_EXCL_STOP
      #endif
      return (unsigned) -1;
    }
    case Type::OR: {
      for (DNLID termId = instance.getTermIndexes().first;
           termId <= instance.getTermIndexes().second; termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (constants1_.find(term.getIsoID()) != constants1_.end()) {
          #ifdef DEBUG_PRINTS
          // LCOV_EXCL_START
          printf("p OR 1\n");
          // LCOV_EXCL_STOP
          #endif
          return 1;
        }
      }
      #ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("p OR nan\n");
      // LCOV_EXCL_STOP
      #endif
      return (unsigned) -1;
    }
    case Type::NAND: {
      for (DNLID termId = instance.getTermIndexes().first;
           termId <= instance.getTermIndexes().second; termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (constants0_.find(term.getIsoID()) != constants0_.end()) {
          #ifdef DEBUG_PRINTS
          // LCOV_EXCL_START
          printf("p NAND 1\n");
          // LCOV_EXCL_STOP
          #endif
          return 1;
        }
      }
      #ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("p NAND nan\n");
      // LCOV_EXCL_STOP
      #endif
      return (unsigned) -1;
    }
    case Type::NOR: {
      for (DNLID termId = instance.getTermIndexes().first;
           termId <= instance.getTermIndexes().second; termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (constants1_.find(term.getIsoID()) != constants1_.end()) {
          #ifdef DEBUG_PRINTS
          // LCOV_EXCL_START
          printf("p NOR 0\n");
          // LCOV_EXCL_STOP
          #endif
          return 0;
        }
      }
      #ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("p NOR nan\n");
      // LCOV_EXCL_STOP
      #endif
      return (unsigned) -1;
    }
    case Type::DFF: {
      unsigned d = (unsigned) -1;
      for (DNLID termId = instance.getTermIndexes().first;
           termId <= instance.getTermIndexes().second; termId++) {
        const DNLTerminalFull& term = dnl_->getDNLTerminalFromID(termId);
        if (term.getSnlBitTerm()->getName().getString() == std::string("D")) {
          if (constants1_.find(term.getIsoID()) != constants1_.end()) {
            #ifdef DEBUG_PRINTS
            // LCOV_EXCL_START
            printf("p DFF 1\n");
            // LCOV_EXCL_STOP
            #endif
            d = 1;
          } else if (constants0_.find(term.getIsoID()) != constants0_.end()) {
            #ifdef DEBUG_PRINTS
            // LCOV_EXCL_START
            printf("p DFF 0\n");
            // LCOV_EXCL_STOP
            #endif
            d = 0;
          }
          break;
        }
      }
      return d;
    }
    default:
      break;
  }
  return (unsigned)-1;
}

void ConstantPropagation::changeDriverToLocal0(SNLInstTerm* term, DNLID id) {
  term->setNet(nullptr);
  SNLNet* assign0 =
      SNLScalarNet::create(term->getInstance()->getDesign(),
                           SNLName(std::string("assign0_") +
                                   term->getBitTerm()->getName().getString()  +std::to_string(term->getBitTerm()->getBit())+ std::to_string(id)));
  assign0->setType(naja::SNL::SNLNet::Type::Assign0);
  term->setNet(assign0);
}

void ConstantPropagation::changeDriverToLocal1(SNLInstTerm* term, DNLID id) {
  term->setNet(nullptr);
  SNLNet* assign1 =
      SNLScalarNet::create(term->getInstance()->getDesign(),
                           SNLName(std::string("assign1_") +
                                   term->getBitTerm()->getName().getString() +std::to_string(term->getBitTerm()->getBit()) + std::to_string(id)));
  assign1->setType(naja::SNL::SNLNet::Type::Assign1);
  term->setNet(assign1);
}

void ConstantPropagation::propagateConstants() {
  for (DNLID iso : constants0_) {
    DNLIso const0iso = dnl_->getDNLIsoDB().getIsoFromIsoIDconst(iso);
    if (const0iso.getDrivers().size() > 1) {
      continue;
    }
    for (DNLID reader : const0iso.getReaders()) {
      DNLTerminalFull readerTerm = dnl_->getDNLTerminalFromID(reader);
      DNLInstanceFull readerInst = readerTerm.getDNLInstance();
      if (readerInst.isTop()) {
        constant0TopReaders_.push_back(readerTerm.getSnlBitTerm());
        continue;
      }
      std::vector<SNLInstance*> path;
      DNLInstanceFull currentInstance = readerInst;
      while (currentInstance.isTop() == false) {
        path.push_back(currentInstance.getSNLInstance());
        currentInstance = currentInstance.getParentInstance();
      }
      std::reverse(path.begin(), path.end());
      constant0Readers_.push_back(
          std::tuple<std::vector<SNLInstance*>, SNLInstTerm*, DNLID>(
              path, readerTerm.getSnlTerm(), readerInst.getID()));
    }
  }
  for (auto& path : constant0Readers_) {
    Uniquifier uniquifier(std::get<0>(path), std::get<2>(path));
    uniquifier.process();
    SNLInstTerm* constTerm = uniquifier.getPathUniq().back()->getInstTerm(
        std::get<1>(path)->getBitTerm());
    changeDriverToLocal0(constTerm, std::get<2>(path));
  }
  for (SNLBitTerm* term : constant0TopReaders_) {
    term->setNet(nullptr);
    SNLNet* assign0 = SNLScalarNet::create(
        term->getDesign(),
        SNLName(std::string("assign0_") + term->getName().getString() +std::to_string(term->getBit())));
    assign0->setType(naja::SNL::SNLNet::Type::Assign0);
    term->setNet(assign0);
  }
  for (DNLID iso : constants1_) {
    DNLIso const1iso = dnl_->getDNLIsoDB().getIsoFromIsoIDconst(iso);
    if (const1iso.getDrivers().size() > 1) {
      continue;
    }
    for (DNLID reader : const1iso.getReaders()) {
      DNLTerminalFull readerTerm = dnl_->getDNLTerminalFromID(reader);
      DNLInstanceFull readerInst = readerTerm.getDNLInstance();
      if (readerInst.isTop()) {
        constant1TopReaders_.push_back(readerTerm.getSnlBitTerm());
        continue;
      }
      std::vector<SNLInstance*> path;
      DNLInstanceFull currentInstance = readerInst;
      while (currentInstance.isTop() == false) {
        path.push_back(currentInstance.getSNLInstance());
        currentInstance = currentInstance.getParentInstance();
      }
      std::reverse(path.begin(), path.end());
      constant1Readers_.push_back(
          std::tuple<std::vector<SNLInstance*>, SNLInstTerm*, DNLID>(
              path, readerTerm.getSnlTerm(), readerInst.getID()));
    }
  }
  for (auto& path : constant1Readers_) {
    Uniquifier uniquifier(std::get<0>(path), std::get<2>(path));
    uniquifier.process();
    SNLInstTerm* constTerm = uniquifier.getPathUniq().back()->getInstTerm(
        std::get<1>(path)->getBitTerm());
    changeDriverToLocal1(constTerm, std::get<2>(path));
  }
  for (SNLBitTerm* term : constant1TopReaders_) {
    term->setNet(nullptr);
    SNLNet* assign1 = SNLScalarNet::create(
        term->getDesign(),
        SNLName(std::string("assign1_") + term->getName().getString() +std::to_string(term->getBit())));
    assign1->setType(naja::SNL::SNLNet::Type::Assign1);
    term->setNet(assign1);
  }
}

void ConstantPropagation::run() {
  initializeTypesID();
  collectConstants();
  performConstantPropagationAnalysis();
  propagateConstants();
  destroy();
}
