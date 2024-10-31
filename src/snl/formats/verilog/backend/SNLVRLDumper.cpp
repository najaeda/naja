// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLVRLDumper.h"

#include <cassert>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <unordered_set>

#include "SNLLibrary.h"
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
#include "SNLDB0.h"

namespace {

size_t dumpDirection(const naja::SNL::SNLTerm* term, std::ostream& o) {
  switch (term->getDirection()) {
    case naja::SNL::SNLTerm::Direction::Input:
      o << "input";
      return std::char_traits<char>::length("input");
    case naja::SNL::SNLTerm::Direction::Output:
      o << "output";
      return std::char_traits<char>::length("output");
    case naja::SNL::SNLTerm::Direction::InOut:
      o << "inout";
      return std::char_traits<char>::length("inout");
  }
  return 0; //LCOV_EXCL_LINE
}

using ContiguousNetBits = std::vector<naja::SNL::SNLBitNet*>;
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
      if (bit->getType() == naja::SNL::SNLNet::Type::Assign0) {
        constantStr += "0";
      } else if (bit->getType() == naja::SNL::SNLNet::Type::Assign1) {
        constantStr += "1";
      } else {
        throw naja::SNL::SNLVRLDumperException("ERROR");
      }
    }
    if (constantStr.size() < 4) {
      //binary
      o += std::to_string(bits.size()) + "'b";
      o += constantStr;
    } else {
      //hexa
      o += std::to_string(bits.size()) + "'h";
      constantStr = naja::SNL::SNLVRLDumper::binStrToHexStr(constantStr);
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
      naja::SNL::SNLBusNetBit* rangeMSBBit = static_cast<naja::SNL::SNLBusNetBit*>(bits[0]);
      naja::SNL::SNLID::Bit rangeMSB = rangeMSBBit->getBit();
      naja::SNL::SNLBusNetBit* rangeLSBBit =static_cast<naja::SNL::SNLBusNetBit*>(bits[bits.size()-1]);
      naja::SNL::SNLID::Bit rangeLSB = rangeLSBBit->getBit();
      naja::SNL::SNLBusNet* bus = rangeMSBBit->getBus();
      naja::SNL::SNLID::Bit busMSB = bus->getMSB();
      naja::SNL::SNLID::Bit busLSB = bus->getLSB();
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

std::string getBitNetString(const naja::SNL::SNLBitNet* bitNet) {
  if (auto scalarNet = dynamic_cast<const naja::SNL::SNLScalarNet*>(bitNet)) {
    return dumpName(scalarNet->getName().getString());
  } else {
    auto busNetBit = static_cast<const naja::SNL::SNLBusNetBit*>(bitNet);
    auto bus = busNetBit->getBus();
    auto busName = dumpName(bus->getName().getString());
    return busName + "[" + std::to_string(busNetBit->getBit()) + "]";
  }
}

}

namespace naja { namespace SNL {

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
  while (library->getDesign(SNLName(designName))) {
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
  while (design->getInstance(SNLName(uniqueInstanceName))
      && naming.instanceNameSet_.find(uniqueInstanceName) != naming.instanceNameSet_.end()) {
    uniqueInstanceName = instanceName + "_" + std::to_string(conflict++); 
  }
  naming.instanceNameSet_.insert(uniqueInstanceName);
  naming.instanceNames_[instance->getID()] = uniqueInstanceName;
  return uniqueInstanceName;
}

SNLName SNLVRLDumper::createNetName(const SNLNet* net, DesignInsideAnonymousNaming& naming) {
  auto design = net->getDesign();
  auto netID = net->getID();
  std::string netName = "net_" + std::to_string(netID);
  int conflict = 0;
  SNLName uniqueNetName(netName);
  while (naming.netTermNameSet_.find(SNLName(uniqueNetName)) != naming.netTermNameSet_.end()) {
    uniqueNetName = SNLName(netName + "_" + std::to_string(conflict++)); 
  }
  naming.netTermNameSet_.insert(uniqueNetName);
  naming.netNames_[net->getID()] = uniqueNetName;
  return uniqueNetName;
}

SNLName SNLVRLDumper::getNetName(const SNLNet* net, const DesignInsideAnonymousNaming& naming) {
  if (net->isAnonymous()) {
    auto it = naming.netNames_.find(net->getID());
    assert(it != naming.netNames_.end());
    return it->second;
  } else {
    return net->getName();
  }
}

void SNLVRLDumper::dumpAttributes(const SNLObject* object, std::ostream& o) {
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
  SNLName netName;
  if (net->isAnonymous()) {
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
    if (not net->isAnonymous()) {
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
          SNLName netName = getNetName(net, naming);
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
  if (SNLDB0::isAssign(instance->getModel())) {
    auto inputNet = instance->getInstTerm(SNLDB0::getAssignInput())->getNet();
    auto outputNet = instance->getInstTerm(SNLDB0::getAssignOutput())->getNet();
    if (inputNet and outputNet) {
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
    } else {
      return false;
    }
  }
  std::string instanceName;
  if (instance->isAnonymous()) {
    instanceName = createInstanceName(instance, naming);
  } else {
    instanceName = instance->getName().getString();
  }
  dumpAttributes(instance, o);
  auto model = instance->getModel();
  if (not model->isAnonymous()) { //FIXME !!
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
  for (auto instance: design->getInstances()) {
    if (blankLine) {
      o << std::endl;
    }
    blankLine = dumpInstance(instance, o, naming);
  }
}

void SNLVRLDumper::dumpTermNetAssign(
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
      throw SNLVRLDumperException("wrong direction in assign");
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
              scalarTerm->getDirection(), scalarTerm->getString(), busNetBit->getString(), o);
          }
        }
      } else {
        auto busTermBit = static_cast<SNLBusTermBit*>(term);
        auto busTerm = busTermBit->getBus();
        if (auto scalarNet = dynamic_cast<SNLScalarNet*>(net)) {
          if (busTerm->getName() == scalarNet->getName()) {
            std::ostringstream reason;
            reason << "Error while writing verilog: bus terminal ";
            reason << busTerm->getString();
            reason << " and scalar net ";
            reason << scalarNet->getString();
            reason << " should not have the same name.";
            throw SNLVRLDumperException(reason.str());
          } else {
            atLeastOne = true;
            dumpTermNetAssign(
              busTerm->getDirection(), busTermBit->getString(), scalarNet->getString(), o);
          }
        } else {
          auto busNetBit = static_cast<SNLBusNetBit*>(net);
          auto busNet = busNetBit->getBus();
          if (busTerm->getName() == busNet->getName()) {
            if (busTermBit->getBit() != busNetBit->getBit()) {
              std::ostringstream reason;
              reason << "Error while writing verilog: bus terminal bit";
              reason << busTermBit->getString();
              reason << " and scalar net bit ";
              reason << busNetBit->getString();
              reason << " should have the same bit value.";
              throw SNLVRLDumperException(reason.str());
            }
          } else {
            atLeastOne = true;
            dumpTermNetAssign(
              busTerm->getDirection(), busTermBit->getString(), busNetBit->getString(), o);
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
    if (not term->isAnonymous()) {
      naming.netTermNameSet_.insert(term->getName());
    }
  }
  for (auto net: design->getNets()) {
    if (not net->isAnonymous()) {
      naming.netTermNameSet_.insert(net->getName());
    }
  }
  if (design->isAnonymous()) {
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

void SNLVRLDumper::dumpLibrary(const SNLLibrary* library, std::ostream& o) {
  for (auto design: library->getDesigns()) {
    dumpOneDesign(design, o);
  }
}

std::string SNLVRLDumper::getTopFileName(const SNLDesign* top) const {
  if (configuration_.hasTopFileName()) {
    return configuration_.getTopFileName();
  }
  if (not top->isAnonymous()) {
    return top->getName().getString() + ".v";
  }
  return "top.v";
} 

std::string SNLVRLDumper::getLibraryFileName(const SNLLibrary* library) const {
  if (configuration_.hasLibraryFileName()) {
    return configuration_.getLibraryFileName();
  }
  if (not library->isAnonymous()) {
    return library->getName().getString() + ".v";
  }
  return "library.v";
} 

void SNLVRLDumper::dumpDesign(const SNLDesign* design, const std::filesystem::path& path) {
  if (not std::filesystem::exists(path)) {
    std::ostringstream reason;
    if (not design->isAnonymous()) {
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
      streamDumper.dumpDesign(design, outFile);
    }
  }
}

void SNLVRLDumper::dumpLibrary(const SNLLibrary* library, const std::filesystem::path& path) {
  if (not std::filesystem::exists(path)) {
    std::ostringstream reason;
    if (not library->isAnonymous()) {
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
    dumpLibrary(library, outFile);
  } else {
    for (auto design: library->getDesigns()) {
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
      throw naja::SNL::SNLVRLDumperException(reason.str());
    }
    it += 4;
  }
  return hexStr;
}

}} // namespace SNL // namespace naja