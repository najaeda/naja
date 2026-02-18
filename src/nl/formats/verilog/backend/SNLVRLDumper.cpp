// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLVRLDumper.h"

#include <cassert>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <unordered_set>

#include "NajaUtils.h"
#include "NajaPerf.h"

#include "NLDB0.h"
#include "NLLibrary.h"

#include "SNLDesign.h"
#include "SNLParameter.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLInstTerm.h"
#include "SNLAttributes.h"
#include "SNLUtils.h"

namespace {

size_t dumpDirection(const naja::NL::SNLTerm* term, std::ostream& o) {
  switch (term->getDirection()) {
    case naja::NL::SNLTerm::Direction::Input:
      o << "input";
      return std::char_traits<char>::length("input");
    case naja::NL::SNLTerm::Direction::Output:
      o << "output";
      return std::char_traits<char>::length("output");
    case naja::NL::SNLTerm::Direction::InOut:
    case naja::NL::SNLTerm::Direction::Undefined:
      o << "inout";
      return std::char_traits<char>::length("inout");
  }
  return 0; //LCOV_EXCL_LINE
}

using ContiguousNetBits = std::vector<naja::NL::SNLBitNet*>;

char getAssignConstantBitValue(const naja::NL::SNLBitNet* bit) {
  switch (bit->getType()) {
    case naja::NL::SNLNet::Type::Assign0:
      return '0';
    case naja::NL::SNLNet::Type::Assign1:
      return '1';
    default:
      throw naja::NL::SNLVRLDumperException("ERROR"); //LCOV_EXCL_LINE
  }
}

void dumpConstantRange(ContiguousNetBits& bits, bool& firstElement, bool& concatenation, std::string& o) {
  if (not bits.empty()) {
    if (not firstElement) {
      o += ", ";
      concatenation = true;
    } else {
      firstElement = false;
    }
    std::string constantStr;
    for (auto bit: bits) {
      constantStr += getAssignConstantBitValue(bit);
    }
    if (constantStr.size() < 4) {
      //binary
      o += std::to_string(bits.size()) + "'b";
      o += constantStr;
    } else {
      //hexa
      o += std::to_string(bits.size()) + "'h";
      constantStr = naja::NL::SNLVRLDumper::binStrToHexStr(constantStr);
      o += constantStr;
    }
  }
  bits.clear();
}

std::string dumpName(const std::string& name) {
  // An identifier is used to give an object a unique name so it can be referenced.
  // An identifier is either a simple identifier or an escaped identifier (see 3.7.1).
  // A simple identifier shall be any sequence of letters, digits, dollar signs ($),
  // and underscore characters (_).
  // The first character of a simple identifier shall not be a digit or $; 
  // it can be a letter or an underscore. Identifiers shall be case sensitive.

  //special first character case
  bool escape = (name[0] == '$' or ('0' <= name[0] and name[0] <= '9'));
  if (not escape) {
    for (int i=0; i<name.size(); ++i) {
      if ('0' <= name[i] && name[i] <= '9') {
        continue;
      }
      if ('a' <= name[i] && name[i] <= 'z') {
        continue;
      }
      if ('A' <= name[i] && name[i] <= 'Z') {
        continue;
      }
      if (name[i] == '_') {
        continue;
      }
      if (name[i] == '$') {
        continue;
      }
      escape = true;
      break;
    }
  }

  static const std::unordered_set<std::string> keywords = {
		// IEEE 1800-2017 Annex B
	  "accept_on", "alias", "always", "always_comb", "always_ff", "always_latch", "and", "assert", "assign", "assume", "automatic", "before",
		"begin", "bind", "bins", "binsof", "bit", "break", "buf", "bufif0", "bufif1", "byte", "case", "casex", "casez", "cell", "chandle",
		"checker", "class", "clocking", "cmos", "config", "const", "constraint", "context", "continue", "cover", "covergroup", "coverpoint",
		"cross", "deassign", "default", "defparam", "design", "disable", "dist", "do", "edge", "else", "end", "endcase", "endchecker",
		"endclass", "endclocking", "endconfig", "endfunction", "endgenerate", "endgroup", "endinterface", "endmodule", "endpackage",
		"endprimitive", "endprogram", "endproperty", "endsequence", "endspecify", "endtable", "endtask", "enum", "event", "eventually",
		"expect", "export", "extends", "extern", "final", "first_match", "for", "force", "foreach", "forever", "fork", "forkjoin", "function",
		"generate", "genvar", "global", "highz0", "highz1", "if", "iff", "ifnone", "ignore_bins", "illegal_bins", "implements", "implies",
		"import", "incdir", "include", "initial", "inout", "input", "inside", "instance", "int", "integer", "interconnect", "interface",
		"intersect", "join", "join_any", "join_none", "large", "let", "liblist", "library", "local", "localparam", "logic", "longint",
		"macromodule", "matches", "medium", "modport", "module", "nand", "negedge", "nettype", "new", "nexttime", "nmos", "nor",
		"noshowcancelled", "not", "notif0", "notif1", "null", "or", "output", "package", "packed", "parameter", "pmos", "posedge", "primitive",
		"priority", "program", "property", "protected", "pull0", "pull1", "pulldown", "pullup", "pulsestyle_ondetect", "pulsestyle_onevent",
		"pure", "rand", "randc", "randcase", "randsequence", "rcmos", "real", "realtime", "ref", "reg", "reject_on", "release", "repeat",
		"restrict", "return", "rnmos", "rpmos", "rtran", "rtranif0", "rtranif1", "s_always", "s_eventually", "s_nexttime", "s_until",
		"s_until_with", "scalared", "sequence", "shortint", "shortreal", "showcancelled", "signed", "small", "soft", "solve", "specify",
		"specparam", "static", "string", "strong", "strong0", "strong1", "struct", "super", "supply0", "supply1", "sync_accept_on",
		"sync_reject_on", "table", "tagged", "task", "this", "throughout", "time", "timeprecision", "timeunit", "tran", "tranif0", "tranif1",
		"tri", "tri0", "tri1", "triand", "trior", "trireg", "type", "typedef", "union", "unique", "unique0", "unsigned", "until", "until_with",
		"untyped", "use", "uwire", "var", "vectored", "virtual", "void", "wait", "wait_order", "wand", "weak", "weak0", "weak1", "while",
		"wildcard", "wire", "with", "within", "wor", "xnor", "xor",
	};
	if (keywords.find(name) != keywords.end()) {
		escape = true;
  }

  if (escape) {
    return "\\" + name + " ";
  }
  return name;
}

void dumpRange(ContiguousNetBits& bits, bool& firstElement, bool& concatenation, std::string& o) {
  if (not bits.empty()) {
    if (bits[0]->isAssignConstant()) {
      dumpConstantRange(bits, firstElement, concatenation, o);
    } else {
      if (not firstElement) {
        o += ", ";
        concatenation = true;
      } else {
        firstElement = false;
      }
      naja::NL::SNLBusNetBit* rangeMSBBit = static_cast<naja::NL::SNLBusNetBit*>(bits[0]);
      naja::NL::NLID::Bit rangeMSB = rangeMSBBit->getBit();
      naja::NL::SNLBusNetBit* rangeLSBBit =static_cast<naja::NL::SNLBusNetBit*>(bits[bits.size()-1]);
      naja::NL::NLID::Bit rangeLSB = rangeLSBBit->getBit();
      naja::NL::SNLBusNet* bus = rangeMSBBit->getBus();
      naja::NL::NLID::Bit busMSB = bus->getMSB();
      naja::NL::NLID::Bit busLSB = bus->getLSB();
      if (rangeMSB == busMSB and rangeLSB == busLSB) {
        o += dumpName(bus->getName().getString());
      } else if (rangeMSB == rangeLSB) {
        o += dumpName(bus->getName().getString()) + "[";
        o += std::to_string(rangeMSB);
        o += "]";
      } else {
        o += dumpName(bus->getName().getString()) + "[";
        o += std::to_string(rangeMSB);
        o += ":";
        o += std::to_string(rangeLSB);
        o += "]";
      }
      bits.clear();
    }
  }
}

std::string getBitNetString(const naja::NL::SNLBitNet* bitNet) {
  if (auto scalarNet = dynamic_cast<const naja::NL::SNLScalarNet*>(bitNet)) {
    return dumpName(scalarNet->getName().getString());
  } else {
    auto busNetBit = static_cast<const naja::NL::SNLBusNetBit*>(bitNet);
    auto bus = busNetBit->getBus();
    auto busName = dumpName(bus->getName().getString());
    return busName + "[" + std::to_string(busNetBit->getBit()) + "]";
  }
}

std::string getBusBitConcatenationString(const std::vector<const naja::NL::SNLBusNetBit*>& bits) {
  assert(not bits.empty());
  std::string concatenation = "{";
  for (size_t i = 0; i < bits.size(); ++i) {
    if (i != 0) {
      concatenation += ", ";
    }
    concatenation += getBitNetString(bits[i]);
  }
  concatenation += "}";
  return concatenation;
}

naja::NL::NLID::Bit getCanonicalRangeStep(const naja::NL::SNLBusNet* bus) {
  return (bus->getMSB() > bus->getLSB()) ? -1 : 1;
}

bool isCanonicalBusRangeOrder(const std::vector<const naja::NL::SNLBusNetBit*>& bits) {
  assert(not bits.empty());
  if (bits.size() == 1) {
    return true;
  }
  auto bus = bits.front()->getBus();
  auto expectedStep = getCanonicalRangeStep(bus);
  auto actualStep = bits[1]->getBit() - bits[0]->getBit();
  return actualStep == expectedStep;
}

void normalizeAssignGroupOutputOrder(
  std::vector<const naja::NL::SNLBitNet*>& inputBits,
  std::vector<const naja::NL::SNLBusNetBit*>& outputBits) {
  assert(inputBits.size() == outputBits.size());
  assert(not outputBits.empty());
  if (outputBits.size() == 1) {
    return;
  }
  auto outputBus = outputBits.front()->getBus();
  auto expectedOutputStep = getCanonicalRangeStep(outputBus);
  auto outputStep = outputBits[1]->getBit() - outputBits[0]->getBit();
  if (outputStep != expectedOutputStep) {
    std::reverse(inputBits.begin(), inputBits.end());
    std::reverse(outputBits.begin(), outputBits.end());
  }
}

bool isContiguousBitDelta(const naja::NL::NLID::Bit delta) {
  return delta == 1 or delta == -1;
}

bool getAssignConnectivity(
  const naja::NL::SNLInstance* instance,
  const naja::NL::SNLBitNet*& inputNet,
  const naja::NL::SNLBitNet*& outputNet) {
  inputNet = nullptr;
  outputNet = nullptr;
  auto inputTerm = instance->getInstTerm(naja::NL::NLDB0::getAssignInput());
  auto outputTerm = instance->getInstTerm(naja::NL::NLDB0::getAssignOutput());
  if (inputTerm and outputTerm) {
    inputNet = inputTerm->getNet();
    outputNet = outputTerm->getNet();
  }
  return inputNet and outputNet;
}

bool dumpSingleAssign(
  const naja::NL::SNLBitNet* inputNet,
  const naja::NL::SNLBitNet* outputNet,
  std::ostream& o) {
  std::string inputNetString;
  if (inputNet->isConstant0()) {
    inputNetString = "1'b0";
  } else if (inputNet->isConstant1()) {
    inputNetString = "1'b1";
  } else {
    inputNetString = getBitNetString(inputNet);
  }
  auto outputNetString = getBitNetString(outputNet);
  o << "assign " << outputNetString << " = " << inputNetString << ";" << std::endl;
  return true;
}

std::string getBusBitRangeString(const std::vector<const naja::NL::SNLBusNetBit*>& bits) {
  assert(not bits.empty());
  auto firstBit = bits.front();
  auto lastBit = bits.back();
  auto bus = firstBit->getBus();
  assert(bus == lastBit->getBus());
  auto busName = dumpName(bus->getName().getString());
  if (firstBit->getBit() == bus->getMSB() and lastBit->getBit() == bus->getLSB()) {
    return busName;
  }
  return busName + "[" + std::to_string(firstBit->getBit()) + ":" + std::to_string(lastBit->getBit()) + "]";
}

std::string getAssignConstantString(const std::vector<const naja::NL::SNLBitNet*>& bits) {
  assert(not bits.empty());
  std::string bitString;
  bitString.reserve(bits.size());
  for (auto bit: bits) {
    bitString += getAssignConstantBitValue(bit);
  }
  return std::to_string(bits.size()) + "'b" + bitString;
}

enum class AssignInputMode {
  Bus,
  Constant
};

struct AssignGroup {
  AssignInputMode                              inputMode_ {AssignInputMode::Bus};
  std::vector<const naja::NL::SNLBitNet*>      inputBits_ {};
  std::vector<const naja::NL::SNLBusNetBit*>   outputBits_{};
  bool                                          hasOutputStep_{false};
  naja::NL::NLID::Bit                           outputStep_{0};
  bool                                          hasInputStep_ {false};
  naja::NL::NLID::Bit                           inputStep_ {0};
};

bool initializeAssignGroup(
  const naja::NL::SNLBitNet* inputNet,
  const naja::NL::SNLBitNet* outputNet,
  AssignGroup& group) {
  auto outputBusBit = dynamic_cast<const naja::NL::SNLBusNetBit*>(outputNet);
  if (not outputBusBit) {
    return false;
  }
  if (inputNet->isAssignConstant()) {
    group.inputMode_ = AssignInputMode::Constant;
  } else {
    auto inputBusBit = dynamic_cast<const naja::NL::SNLBusNetBit*>(inputNet);
    if (not inputBusBit) {
      return false;
    }
    group.inputMode_ = AssignInputMode::Bus;
  }
  group.inputBits_.push_back(inputNet);
  group.outputBits_.push_back(outputBusBit);
  return true;
}

bool appendAssignGroup(
  AssignGroup& group,
  const naja::NL::SNLBitNet* inputNet,
  const naja::NL::SNLBitNet* outputNet) {
  auto outputBusBit = dynamic_cast<const naja::NL::SNLBusNetBit*>(outputNet);
  if (not outputBusBit) {
    return false;
  }

  auto previousOutputBit = group.outputBits_.back();
  if (outputBusBit->getBus() != previousOutputBit->getBus()) {
    return false;
  }

  auto outputDelta = outputBusBit->getBit() - previousOutputBit->getBit();
  if (not isContiguousBitDelta(outputDelta)) {
    return false;
  }
  if (group.hasOutputStep_ and outputDelta != group.outputStep_) {
    return false;
  }

  if (group.inputMode_ == AssignInputMode::Constant) {
    if (not inputNet->isAssignConstant()) {
      return false;
    }
  } else {
    auto inputBusBit = dynamic_cast<const naja::NL::SNLBusNetBit*>(inputNet);
    if (not inputBusBit) {
      return false;
    }
    auto previousInputBusBit = static_cast<const naja::NL::SNLBusNetBit*>(group.inputBits_.back());
    if (inputBusBit->getBus() != previousInputBusBit->getBus()) {
      return false;
    }
    auto inputDelta = inputBusBit->getBit() - previousInputBusBit->getBit();
    if (not isContiguousBitDelta(inputDelta)) {
      return false;
    }
    if (group.hasInputStep_ and inputDelta != group.inputStep_) {
      return false;
    }
    if (not group.hasInputStep_) {
      group.inputStep_ = inputDelta;
      group.hasInputStep_ = true;
    }
  }

  if (not group.hasOutputStep_) {
    group.outputStep_ = outputDelta;
    group.hasOutputStep_ = true;
  }
  group.inputBits_.push_back(inputNet);
  group.outputBits_.push_back(outputBusBit);
  return true;
}

bool dumpAssignGroup(const AssignGroup& group, std::ostream& o) {
  assert(group.inputBits_.size() == group.outputBits_.size());
  assert(group.outputBits_.size() > 1);
  auto inputBits = group.inputBits_;
  auto outputBits = group.outputBits_;
  normalizeAssignGroupOutputOrder(inputBits, outputBits);
  auto outputString = getBusBitRangeString(outputBits);
  std::string inputString;
  if (group.inputMode_ == AssignInputMode::Constant) {
    inputString = getAssignConstantString(inputBits);
  } else {
    std::vector<const naja::NL::SNLBusNetBit*> inputBusBits;
    inputBusBits.reserve(inputBits.size());
    for (auto inputBit: inputBits) {
      inputBusBits.push_back(static_cast<const naja::NL::SNLBusNetBit*>(inputBit));
    }
    if (isCanonicalBusRangeOrder(inputBusBits)) {
      inputString = getBusBitRangeString(inputBusBits);
    } else {
      inputString = getBusBitConcatenationString(inputBusBits);
    }
  }
  o << "assign " << outputString << " = " << inputString << ";" << std::endl;
  return true;
}

}

namespace naja::NL {

void SNLVRLDumper::setSingleFile(bool mode) {
  configuration_.setSingleFile(mode);
}

void SNLVRLDumper::setLibraryFileName(const std::string& name) {
  configuration_.setLibraryFileName(name);
}

void SNLVRLDumper::setTopFileName(const std::string& name) {
  configuration_.setTopFileName(name);
}

void SNLVRLDumper::setDumpHierarchy(bool mode) {
  configuration_.setDumpHierarchy(mode);
}

std::string SNLVRLDumper::createDesignName(const SNLDesign* design) {
  auto library = design->getLibrary();
  auto designID = design->getID();
  std::string designName = "module" + std::to_string(designID);
  int conflict = 0;
  while (library->getSNLDesign(NLName(designName))) {
    designName += "_" + std::to_string(conflict++); 
  }
  return designName;
}

std::string SNLVRLDumper::createInstanceName(const SNLInstance* instance, DesignInsideAnonymousNaming& naming) {
  auto design = instance->getDesign();
  auto instanceID = instance->getID();
  std::string instanceName = "instance_" + std::to_string(instanceID);
  int conflict = 0;
  std::string uniqueInstanceName(instanceName);
  while (design->getInstance(NLName(uniqueInstanceName))
      && naming.instanceNameSet_.find(uniqueInstanceName) != naming.instanceNameSet_.end()) {
    uniqueInstanceName = instanceName + "_" + std::to_string(conflict++); 
  }
  naming.instanceNameSet_.insert(uniqueInstanceName);
  naming.instanceNames_[instance->getID()] = uniqueInstanceName;
  return uniqueInstanceName;
}

NLName SNLVRLDumper::createNetName(const SNLNet* net, DesignInsideAnonymousNaming& naming) {
  auto design = net->getDesign();
  auto netID = net->getID();
  std::string netName = "net_" + std::to_string(netID);
  int conflict = 0;
  NLName uniqueNetName(netName);
  while (naming.netTermNameSet_.find(NLName(uniqueNetName)) != naming.netTermNameSet_.end()) {
    uniqueNetName = NLName(netName + "_" + std::to_string(conflict++)); 
  }
  naming.netTermNameSet_.insert(uniqueNetName);
  naming.netNames_[net->getID()] = uniqueNetName;
  return uniqueNetName;
}

NLName SNLVRLDumper::getNetName(const SNLNet* net, const DesignInsideAnonymousNaming& naming) {
  if (net->isUnnamed()) {
    auto it = naming.netNames_.find(net->getID());
    assert(it != naming.netNames_.end());
    return it->second;
  } else {
    return net->getName();
  }
}

void SNLVRLDumper::dumpAttributes(const NLObject* object, std::ostream& o) {
  for (const auto& attribute: SNLAttributes::getAttributes(object)) {
    o << "(* ";
    o << attribute.getName().getString();
    if (attribute.hasValue()) {
      if (attribute.getValue().isString()) {
        o << "=\"";
      }
      o << "=" << attribute.getValue().getString();
      if (attribute.getValue().isString()) {
        o << "\"";
      }
    }
    o << " *)" << std::endl;
  }
}

void SNLVRLDumper::dumpInterface(const SNLDesign* design, std::ostream& o, DesignInsideAnonymousNaming& naming) {
  size_t nbChars = std::char_traits<char>::length("module  (");
  nbChars += design->getName().getString().size();
  o << "(";
  bool first = true;
  for (auto term: design->getTerms()) {
    if (not first) {
      o << ",";
      nbChars += 1;
      if (nbChars > 80) {
        o << std::endl;
        nbChars = 0;
      }
      nbChars += 1;
      o << " ";
    } else {
      first = false;
    }
    nbChars += dumpDirection(term, o) + 1;
    o << " ";
    if (auto bus = dynamic_cast<SNLBusTerm*>(term)) {
      o << "[" << bus->getMSB() << ":" << bus->getLSB() << "] ";
      nbChars += 3 + std::to_string(bus->getMSB()).size() + std::to_string(bus->getLSB()).size();
    }
    const auto termName = term->getName().getString();
    nbChars += termName.size();
    o << dumpName(termName);
  }
  o << ");";
}

bool SNLVRLDumper::dumpNet(const SNLNet* net, std::ostream& o, DesignInsideAnonymousNaming& naming) {
  if (net->isAssignConstant()) {
    return false;
  }
  NLName netName;
  if (net->isUnnamed()) {
    netName = createNetName(net, naming);
  } else {
    netName = net->getName();
  }
  dumpAttributes(net, o);
  o << "wire ";
  if (auto bus = dynamic_cast<const SNLBusNet*>(net)) {
    o << "[" << bus->getMSB() << ":" << bus->getLSB() << "] ";
  }
  o << dumpName(netName.getString());
  o << ";" << std::endl;
  return true;
}

void SNLVRLDumper::dumpNets(const SNLDesign* design, std::ostream& o, DesignInsideAnonymousNaming& naming) {
  bool atLeastOne = false;
  for (auto net: design->getNets()) {
    if (not net->isUnnamed()) {
      auto name = net->getName();
      if (design->getTerm(name)) {
        //already dumped
        continue;
      }
    }
    bool dumped = dumpNet(net, o, naming);
    if (dumped) {
      atLeastOne = true;
    }
  }
  if (atLeastOne) {
    o << std::endl;
  }
}

void SNLVRLDumper::dumpInsTermConnectivity(
  const SNLTerm* term,
  BitNetVector& termNets,
  std::ostream& o,
  const DesignInsideAnonymousNaming& naming) {
  if (std::any_of(termNets.begin(), termNets.end(), [](const SNLBitNet* n){ return n != nullptr; })) {
    assert(not termNets.empty());
    bool concatenation = false;
    bool firstElement = true;
    std::string connectionStr;
    ContiguousNetBits contiguousBits;
    for (auto net: termNets) {
      if (net) {
        if (net->isAssignConstant()) {
          if (not contiguousBits.empty()) {
            SNLBitNet* previousBit = contiguousBits.back();
            if (not previousBit->isAssignConstant()) {
              dumpRange(contiguousBits, firstElement, concatenation, connectionStr);
              contiguousBits = { net };
            } else {
              contiguousBits.push_back(net);
            }
          } else {
            contiguousBits.push_back(net);
          }
        } else if (dynamic_cast<SNLScalarNet*>(net)) {
          dumpRange(contiguousBits, firstElement, concatenation, connectionStr);
          NLName netName = getNetName(net, naming);
          if (not firstElement) {
            connectionStr += ", ";
            concatenation = true;
          } else {
            firstElement = false;
          }
          connectionStr += dumpName(netName.getString());
        } else {
          auto busNetBit = static_cast<SNLBusNetBit*>(net);
          auto busNet = busNetBit->getBus();
          if (not contiguousBits.empty()) {
            SNLBitNet* previousBit = contiguousBits.back();
            if (previousBit->isAssignConstant()) {
              dumpRange(contiguousBits, firstElement, concatenation, connectionStr);
              contiguousBits = { busNetBit };
            } else {
              SNLBusNetBit* previousBusBit = static_cast<SNLBusNetBit*>(previousBit);
              if (busNet == previousBusBit->getBus()
                and ((previousBusBit->getBit() == busNetBit->getBit()+1)
                or (previousBusBit->getBit() == busNetBit->getBit()-1))) {
                contiguousBits.push_back(busNetBit);
              } else {
                dumpRange(contiguousBits, firstElement, concatenation, connectionStr);
                contiguousBits = { busNetBit };
              }
            }
          } else {
            contiguousBits.push_back(busNetBit);
          }
        }
      } else {
        dumpRange(contiguousBits, firstElement, concatenation, connectionStr);
        //dump dummy bit
        if (not firstElement) {
          connectionStr += ", ";
          concatenation = true;
        } else {
          firstElement = false;
        }
        connectionStr += "DUMMY";
      }
    }
    if (not contiguousBits.empty()) {
      dumpRange(contiguousBits, firstElement, concatenation, connectionStr);
    }
    o << "  ." + term->getName().getString() + "(";
    if (concatenation) {
      o << "{";
    }
    o << connectionStr;
    if (concatenation) {
      o << "}";
    }
    o << ")";
  } else {
    //should we not dump anything for non connected inst terms ?
    o << "  ." << dumpName(term->getName().getString()) << "()"; 
  }
}

void SNLVRLDumper::dumpInstanceInterface(
  const SNLInstance* instance,
  std::ostream& o,
  const DesignInsideAnonymousNaming& naming) {
  o << " (";
  BitNetVector termNets;
  SNLTerm* previousTerm = nullptr;
  bool first = true;
  for (auto instTerm: instance->getInstTerms()) {
    SNLTerm* currentTerm = nullptr;
    auto bitTerm = instTerm->getBitTerm();
    if (dynamic_cast<SNLScalarTerm*>(bitTerm)) {
      currentTerm = bitTerm;
    } else {
      auto busTermBit = static_cast<SNLBusTermBit*>(bitTerm);
      currentTerm = busTermBit->getBus();
    }
    if (currentTerm == previousTerm) {
      termNets.push_back(instTerm->getNet());
    } else {
      if (previousTerm) {
        //dump previous term connectivity if at least one net is != nullptr
        if (std::any_of(termNets.begin(), termNets.end(),
          [](const SNLBitNet* n){ return n != nullptr; })) {
          if (first) {
            first = false;
          } else {
            o << ",";
          }
          o << std::endl;
          dumpInsTermConnectivity(previousTerm, termNets, o, naming);
        }
      }
      termNets = { instTerm->getNet() };
    }
    previousTerm = currentTerm;
  }
  if (previousTerm) {
    if (std::any_of(termNets.begin(), termNets.end(),
      [](const SNLBitNet* n){ return n != nullptr; })) {
      if (first) {
        first = false;
      } else {
        o << ",";
      }
      o << std::endl;
      dumpInsTermConnectivity(previousTerm, termNets, o, naming);
    }
  }
  o <<  std::endl << ")";
}

void SNLVRLDumper::dumpInstParameters(
  const SNLInstance* instance,
  std::ostream& o) {
  if (not instance->getInstParameters().empty()) {
    bool first = true;
    o << "#(" << std::endl;
    for (auto instParameter: instance->getInstParameters()) {
      if (not first) {
        o << "," << std::endl;
      }
      first = false;
      o << "  ." << instParameter->getName().getString();
      o << "(";
      auto parameter = instParameter->getParameter();
      if (parameter->getType() == SNLParameter::Type::String) {
        o << "\"" << instParameter->getValue() << "\"";
      } else if (parameter->getType() == SNLParameter::Type::Boolean) {
        if (instParameter->getValue()=="0" or instParameter->getValue()=="FALSE") {
          o << "\"FALSE\"";
        } else if (instParameter->getValue()=="1" or instParameter->getValue()=="TRUE") {
          o << "\"TRUE\"";
        } else {
          std::ostringstream reason;
          reason << "Error while writing verilog: in design " << instance->getDesign()->getString();
          reason << ", for instance " << instance->getName().getString();
          reason << ", wrong boolean value in instance parameter " << parameter->getDescription();
          reason << ": " << instParameter->getDescription();
          throw SNLVRLDumperException(reason.str());
        }
      } else {
        o << instParameter->getValue();
      }
      o << ")";
    }
    o << std::endl << ") ";
  }
}

bool SNLVRLDumper::dumpInstance(
  const SNLInstance* instance,
  std::ostream& o,
  DesignInsideAnonymousNaming& naming) {
  if (NLDB0::isGate(instance->getModel())) {
    auto gateName = NLDB0::getGateName(instance->getModel());
    o << gateName << " ";
    if (not instance->isUnnamed()) {
      o << instance->getName().getString();
    }
    o << "(";
    bool first = true;
    for (auto instTerm: instance->getInstTerms()) {
      if (first) {
        first = false;
      } else {
        o << ", ";
      }
      auto net = instTerm->getNet();
      if (net) {
        o << getBitNetString(net);
      } else {
        o << "DUMMY";
      }
    }
    o << ");";
    o << std::endl;
    return true;
  }
  assert(not NLDB0::isAssign(instance->getModel()));
  std::string instanceName;
  if (instance->isUnnamed()) {
    instanceName = createInstanceName(instance, naming);
  } else {
    instanceName = instance->getName().getString();
  }
  dumpAttributes(instance, o);
  auto model = instance->getModel();
  if (not model->isUnnamed()) { //FIXME !!
    o << dumpName(model->getName().getString()) << " ";
  }
  dumpInstParameters(instance, o);
  o << dumpName(instanceName);
  dumpInstanceInterface(instance, o, naming);
  o << ";" << std::endl;
  return true;
}

void SNLVRLDumper::dumpInstances(const SNLDesign* design, std::ostream& o, DesignInsideAnonymousNaming& naming) {
  bool blankLine = false;
  auto dumpAssignInstances = [&](const std::vector<const SNLInstance*>& assignInstances) {
    size_t i = 0;
    while (i < assignInstances.size()) {
      const SNLBitNet* inputNet = nullptr;
      const SNLBitNet* outputNet = nullptr;
      if (not getAssignConnectivity(assignInstances[i], inputNet, outputNet)) {
        ++i;
        continue;
      }
      AssignGroup group;
      if (not initializeAssignGroup(inputNet, outputNet, group)) {
        if (blankLine) {
          o << std::endl;
        }
        blankLine = dumpSingleAssign(inputNet, outputNet, o);
        ++i;
        continue;
      }

      size_t groupSize = 1;
      while (i + groupSize < assignInstances.size()) {
        const SNLBitNet* nextInputNet = nullptr;
        const SNLBitNet* nextOutputNet = nullptr;
        if (not getAssignConnectivity(assignInstances[i + groupSize], nextInputNet, nextOutputNet)) {
          break;
        }
        if (not appendAssignGroup(group, nextInputNet, nextOutputNet)) {
          break;
        }
        ++groupSize;
      }

      if (group.outputBits_.size() > 1) {
        if (blankLine) {
          o << std::endl;
        }
        blankLine = dumpAssignGroup(group, o);
      } else {
        if (blankLine) {
          o << std::endl;
        }
        blankLine = dumpSingleAssign(inputNet, outputNet, o);
      }
      i += groupSize;
    }
  };

  std::vector<const SNLInstance*> assignInstances;
  for (auto instance: design->getInstances()) {
    if (NLDB0::isAssign(instance->getModel())) {
      assignInstances.push_back(instance);
      continue;
    }
    if (not assignInstances.empty()) {
      dumpAssignInstances(assignInstances);
      assignInstances.clear();
    }
    if (blankLine) {
      o << std::endl;
    }
    blankLine = dumpInstance(instance, o, naming);
  }
  if (not assignInstances.empty()) {
    dumpAssignInstances(assignInstances);
  }
}

void SNLVRLDumper::dumpTermNetAssign(
  const SNLDesign* design,
  const SNLTerm::Direction& direction,
  const std::string& termNetName,
  const std::string& netName,
  std::ostream& o) {
    switch (direction) {
      case SNLTerm::Direction::Input:
        o << "assign " << netName << " = " << termNetName << ";" << std::endl;
        break;
      case SNLTerm::Direction::Output:
        o << "assign " << termNetName << " = " << netName << ";" << std::endl;
        break;
      default:
        {
          std::ostringstream reason;
          reason << "Error while writing verilog of design " << design->getString();
          reason << ", wrong direction (";
          reason << direction.getString() << ") in assign for dumping: ";
          reason << "assign " << termNetName << " = " << netName;
          throw SNLVRLDumperException(reason.str());
        }
    }
}

void SNLVRLDumper::dumpTermAssigns(const SNLDesign* design, std::ostream& o) {
  bool atLeastOne = false;
  for (auto term: design->getBitTerms()) {
    auto net = term->getNet();
    if (net) {
      if (auto scalarTerm = dynamic_cast<SNLScalarTerm*>(term)) {
        if (auto scalarNet = dynamic_cast<SNLScalarNet*>(net)) {
          //same name ?
          if (scalarTerm->getName() != scalarNet->getName()) {
            //need assign
            atLeastOne = true;
            dumpTermNetAssign(
              design,
              scalarTerm->getDirection(),
              scalarTerm->getName().getString(),
              scalarNet->getName().getString(),
              o);
          }
          //else: same name so already connected
        } else {
          auto busNetBit = static_cast<SNLBusNetBit*>(net);
          auto bus = busNetBit->getBus();
          if (scalarTerm->getName() == bus->getName()) {
            std::ostringstream reason;
            reason << "Error while writing verilog: scalar terminal ";
            reason << scalarTerm->getString();
            reason << " and bus net ";
            reason << bus->getString();
            reason << " should not have the same name.";
            throw SNLVRLDumperException(reason.str());
          } else {
            //need assign
            atLeastOne = true;
            dumpTermNetAssign(
              design,
              scalarTerm->getDirection(),
              scalarTerm->getString(),
              busNetBit->getString(),
              o);
          }
        }
      } else {
        auto busTermBit = static_cast<SNLBusTermBit*>(term);
        auto busTerm = busTermBit->getBus();
        if (auto scalarNet = dynamic_cast<SNLScalarNet*>(net)) {
          if (busTerm->getName() == scalarNet->getName()) {
            std::ostringstream reason;
            reason << "Error while writing verilog in design ";
            reason << design->getString() << ": ";
            reason << " bus terminal ";
            reason << busTerm->getString();
            reason << " and scalar net ";
            reason << scalarNet->getString();
            reason << " should not have the same name.";
            throw SNLVRLDumperException(reason.str());
          } else {
            atLeastOne = true;
            dumpTermNetAssign(
              design,
              busTerm->getDirection(),
              busTermBit->getString(),
              scalarNet->getString(),
              o);
          }
        } else {
          auto busNetBit = static_cast<SNLBusNetBit*>(net);
          auto busNet = busNetBit->getBus();
          if (busTerm->getName() == busNet->getName()) {
            if (busTermBit->getBit() != busNetBit->getBit()) {
              std::ostringstream reason;
              reason << "Error while writing verilog in design ";
              reason << design->getString() << ":";
              reason << " bus terminal bit ";
              reason << busTermBit->getString();
              reason << " and bus net bit ";
              reason << busNetBit->getString();
              reason << " should have the same bit value.";
              throw SNLVRLDumperException(reason.str());
            }
          } else {
            atLeastOne = true;
            dumpTermNetAssign(
              design,
              busTerm->getDirection(),
              busTermBit->getString(),
              busNetBit->getString(),
              o
            );
          }
        }
      }
    }
  }
  if (atLeastOne) {
    o << std::endl;
  }
}

void SNLVRLDumper::dumpParameter(const SNLParameter* parameter, std::ostream& o) {
  o << "parameter " << parameter->getName().getString() << " = ";
  if (parameter->getType()==SNLParameter::Type::String) {
    o << "\"" << parameter->getValue() << "\"";
  } else if (parameter->getType()==SNLParameter::Type::Boolean) {
    if (parameter->getValue()=="0") {
      o << "\"FALSE\"";
    } else if (parameter->getValue()=="1") {
      o << "\"TRUE\"";
    } else {
      std::ostringstream reason;
      reason << "Error while writing verilog: in design " << parameter->getDesign()->getString();
      reason << ", wrong boolean value in parameter " << parameter->getDescription();
      throw SNLVRLDumperException(reason.str());
    }
  } else {
    o << parameter->getValue();
  }
  o << " ;" << std::endl;
}

void SNLVRLDumper::dumpParameters(const SNLDesign* design, std::ostream& o) {
  bool atLeastOne = false;
  for (auto parameter: design->getParameters()) {
    atLeastOne = true;
    dumpParameter(parameter, o);
  }
  if (atLeastOne) {
    o << std::endl;
  }
}

void SNLVRLDumper::dumpOneDesign(const SNLDesign* design, std::ostream& o) {
  DesignInsideAnonymousNaming naming;
  for (auto term: design->getTerms()) {
    if (not term->isUnnamed()) {
      naming.netTermNameSet_.insert(term->getName());
    }
  }
  for (auto net: design->getNets()) {
    if (not net->isUnnamed()) {
      naming.netTermNameSet_.insert(net->getName());
    }
  }
  if (design->isUnnamed()) {
    createDesignName(design);
  }
  dumpAttributes(design, o);
  o << "module " << dumpName(design->getName().getString());

  dumpInterface(design, o, naming);
  o << std::endl;

  dumpParameters(design, o);
  dumpNets(design, o, naming);
  dumpTermAssigns(design, o);

  dumpInstances(design, o, naming);

  o << "endmodule //" << design->getName().getString();
  o << std::endl;
}

void SNLVRLDumper::dumpDesign(const SNLDesign* design, std::ostream& o) {
  if (configuration_.isDumpHierarchy()) {
    SNLUtils::SortedDesigns designs;
    SNLUtils::getDesignsSortedByHierarchicalLevel(design, designs);
    bool first = true;
    for (auto designLevel: designs) {
      const SNLDesign* design = designLevel.first;
      if (not design->isPrimitive()) {
        if (not first) {
          o << std::endl;
        }
        first = false;
        dumpOneDesign(design, o);
      }
    }
  } else {
    dumpOneDesign(design, o);
  }
}

void SNLVRLDumper::dumpLibrary(const NLLibrary* library, std::ostream& o) {
  for (auto design: library->getSNLDesigns()) {
    dumpOneDesign(design, o);
  }
}

std::string SNLVRLDumper::getTopFileName(const SNLDesign* top) const {
  if (configuration_.hasTopFileName()) {
    return configuration_.getTopFileName();
  }
  if (not top->isUnnamed()) {
    return top->getName().getString() + ".v";
  }
  return "top.v";
} 

std::string SNLVRLDumper::getLibraryFileName(const NLLibrary* library) const {
  if (configuration_.hasLibraryFileName()) {
    return configuration_.getLibraryFileName();
  }
  if (not library->isUnnamed()) {
    return library->getName().getString() + ".v";
  }
  return "library.v";
} 

void SNLVRLDumper::dumpDesign(const SNLDesign* design, const std::filesystem::path& path) {
  NajaPerf::Scope scope("SNLVRLDumper::dumpDesign");
  if (not std::filesystem::exists(path)) {
    std::ostringstream reason;
    if (not design->isUnnamed()) {
      reason << design->getName().getString();
    } else {
      reason << "anonymous design";
    }
    reason << " cannot be dumped: ";
    reason << path.string() << " " << " does not exist";
    throw SNLVRLDumperException(reason.str());
  }
  if (configuration_.isSingleFile()) {
    //create file
    std::filesystem::path filePath = path/getTopFileName(design);
    std::ofstream outFile;
    outFile.open(filePath);
    NajaUtils::createBanner(
      outFile,
      "Verilog file for " + design->getName().getString(),
      "//"
    );
    outFile << std::endl;
    dumpDesign(design, outFile);
  } else {
    SNLVRLDumper streamDumper;
    SNLVRLDumper::Configuration configuration(configuration_);
    configuration.setDumpHierarchy(false);
    streamDumper.setConfiguration(configuration);
    SNLUtils::SortedDesigns designs;
    SNLUtils::getDesignsSortedByHierarchicalLevel(design, designs);
    for (auto designLevel: designs) {
      const SNLDesign* design = designLevel.first;
      std::filesystem::path filePath = path/getTopFileName(design);
      std::ofstream outFile;
      outFile.open(filePath);
      NajaUtils::createBanner(
        outFile,
        "Verilog file for " + design->getName().getString(),
        "//"
      );
      outFile << std::endl;
      streamDumper.dumpDesign(design, outFile);
    }
  }
}

void SNLVRLDumper::dumpLibrary(const NLLibrary* library, const std::filesystem::path& path) {
  if (not std::filesystem::exists(path)) {
    std::ostringstream reason;
    if (not library->isUnnamed()) {
      reason << library->getName().getString();
    } else {
      reason << library->getDescription();
    }
    reason << " cannot be dumped: path ";
    reason << path.string() << " does not exist";
    throw SNLVRLDumperException(reason.str());
  }
  if (configuration_.isSingleFile()) {
    //create file
    std::filesystem::path filePath = path/getLibraryFileName(library);
    std::ofstream outFile;
    outFile.open(filePath);
    NajaUtils::createBanner(
      outFile,
      "Verilog file for " + library->getName().getString(),
      "//"
    );
    outFile << std::endl;
    dumpLibrary(library, outFile);
  } else {
    for (auto design: library->getSNLDesigns()) {
      dumpDesign(design, path);
    }
  }
}

std::string SNLVRLDumper::binStrToHexStr(std::string binStr) {
  size_t missingZeros = 4-binStr.size()%4;
  for (size_t i=0; i<missingZeros; i++) {
    binStr = '0' + binStr;
  }
  std::string hexStr;
  size_t it = 0;
  while (it < binStr.size()) {
    std::string hex = binStr.substr(it, 4);
    if      (hex == "0000") { hexStr += "0"; }
    else if (hex == "0001") { hexStr += "1"; }
    else if (hex == "0010") { hexStr += "2"; }
    else if (hex == "0011") { hexStr += "3"; }
    else if (hex == "0100") { hexStr += "4"; }
    else if (hex == "0101") { hexStr += "5"; }
    else if (hex == "0110") { hexStr += "6"; }
    else if (hex == "0111") { hexStr += "7"; }
    else if (hex == "1000") { hexStr += "8"; }
    else if (hex == "1001") { hexStr += "9"; }
    else if (hex == "1010") { hexStr += "A"; }
    else if (hex == "1011") { hexStr += "B"; }
    else if (hex == "1100") { hexStr += "C"; }
    else if (hex == "1101") { hexStr += "D"; }
    else if (hex == "1110") { hexStr += "E"; }
    else if (hex == "1111") { hexStr += "F"; }
    else { 
      std::ostringstream reason;
      reason << "Error in binary to hexadecimal conversion: ";
      reason << hex << " is not a convertible binary.";
      throw naja::NL::SNLVRLDumperException(reason.str());
    }
    it += 4;
  }
  return hexStr;
}

} // namespace naja::NL
