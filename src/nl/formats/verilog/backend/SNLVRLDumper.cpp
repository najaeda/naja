// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLVRLDumper.h"

#include <cassert>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <unordered_set>

#include "NajaLog.h"
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
#include "SNLDesignObject.h"
#include "SNLRTLInfos.h"
#include "SNLUtils.h"

namespace {

// LCOV_EXCL_START
double toMilliseconds(const std::chrono::nanoseconds& duration) {
  return std::chrono::duration<double, std::milli>(duration).count();
}
// LCOV_EXCL_STOP

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

bool isUnsignedDecimal(const std::string& value) {
  return std::all_of(
    value.begin(),
    value.end(),
    [](unsigned char c) { return std::isdigit(c) != 0; });
}

std::string normalizeParameterValue(
  naja::NL::SNLParameter::Type type,
  const std::string& value) {
  if (type != naja::NL::SNLParameter::Type::Boolean) {
    return value;
  }
  if (value == "0" || value == "FALSE") {
    return "0";
  }
  if (value == "1" || value == "TRUE") {
    return "1";
  }
  return value;
}

std::string getEmittedDefaultParameterValue(
  const naja::NL::SNLInstance* instance,
  const naja::NL::SNLParameter* parameter) {
  using namespace naja::NL;
  const auto& parameterName = parameter->getName();
  if (NLDB0::isMux2(instance->getModel()) && parameterName == NLName("WIDTH")) {
    return "1";
  }
  if (NLDB0::isMemory(instance->getModel())) {
    if (parameterName == NLName("WIDTH") ||
        parameterName == NLName("DEPTH") ||
        parameterName == NLName("ABITS") ||
        parameterName == NLName("RD_PORTS") ||
        parameterName == NLName("WR_PORTS")) {
      return "1";
    }
    if (parameterName == NLName("RST_ENABLE") ||
        parameterName == NLName("RST_ASYNC") ||
        parameterName == NLName("RST_ACTIVE_LOW")) {
      return "0";
    }
    if (parameterName == NLName("INIT")) {
      return "1'b0";
    }
  }
  return parameter->getValue();
}

bool shouldDumpInstParameter(
  const naja::NL::SNLInstance* instance,
  const naja::NL::SNLInstParameter* instParameter) {
  const auto* parameter = instParameter->getParameter();
  return normalizeParameterValue(parameter->getType(), instParameter->getValue()) !=
         normalizeParameterValue(
           parameter->getType(),
           getEmittedDefaultParameterValue(instance, parameter));
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

std::string getBusBitConcatenationString(
  const std::vector<const naja::NL::SNLBusNetBit*>& bits,
  const auto& bitNetToString) {
  assert(not bits.empty());
  std::string concatenation = "{";
  for (size_t i = 0; i < bits.size(); ++i) {
    if (i != 0) {
      concatenation += ", ";
    }
    concatenation += bitNetToString(bits[i]);
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
  std::ostream& o,
  const auto& bitNetToString) {
  std::string inputNetString;
  if (inputNet->isConstant0()) {
    inputNetString = "1'b0";
  } else if (inputNet->isConstant1()) {
    inputNetString = "1'b1";
  } else {
    inputNetString = bitNetToString(inputNet);
  }
  auto outputNetString = bitNetToString(outputNet);
  o << "assign " << outputNetString << " = " << inputNetString << ";" << '\n';
  return true;
}

std::string getBusBitRangeString(
  const std::vector<const naja::NL::SNLBusNetBit*>& bits,
  const auto& busNetToString) {
  assert(not bits.empty());
  auto firstBit = bits.front();
  auto lastBit = bits.back();
  auto bus = firstBit->getBus();
  assert(bus == lastBit->getBus());
  auto busName = busNetToString(bus);
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

bool dumpAssignGroup(
  const AssignGroup& group,
  std::ostream& o,
  const auto& bitNetToString,
  const auto& busNetToString) {
  assert(group.inputBits_.size() == group.outputBits_.size());
  assert(group.outputBits_.size() > 1);
  auto inputBits = group.inputBits_;
  auto outputBits = group.outputBits_;
  normalizeAssignGroupOutputOrder(inputBits, outputBits);
  auto outputString = getBusBitRangeString(outputBits, busNetToString);
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
      inputString = getBusBitRangeString(inputBusBits, busNetToString);
    } else {
      inputString = getBusBitConcatenationString(inputBusBits, bitNetToString);
    }
  }
  o << "assign " << outputString << " = " << inputString << ";" << '\n';
  return true;
}

}

namespace naja::NL {

SNLVRLDumper::SNLVRLDumper() {
  initializeDetailedPerfConfig();
}

SNLVRLDumper::DetailedPerfScopedTimer::DetailedPerfScopedTimer(
  DetailedPerfReport& report,
  std::chrono::nanoseconds& bucket,
  size_t& calls):
  report_((report.enabled && report.sessionActive) ? &report : nullptr),
  bucket_((report.enabled && report.sessionActive) ? &bucket : nullptr) {
  if (report_) {
    // LCOV_EXCL_START
    ++calls;
    start_ = std::chrono::steady_clock::now();
    // LCOV_EXCL_STOP
  }
}

SNLVRLDumper::DetailedPerfScopedTimer::~DetailedPerfScopedTimer() {
  if (report_) {
    *bucket_ += std::chrono::steady_clock::now() - start_; // LCOV_EXCL_LINE
  }
}

SNLVRLDumper::DetailedPerfSessionGuard::DetailedPerfSessionGuard(
  SNLVRLDumper& dumper,
  const std::string& context):
  dumper_(&dumper),
  started_(dumper.beginDetailedPerfSession(context)) {}

SNLVRLDumper::DetailedPerfSessionGuard::~DetailedPerfSessionGuard() {
  if (dumper_ && started_) {
    dumper_->finalizeDetailedPerfSession(); // LCOV_EXCL_LINE
  }
}

void SNLVRLDumper::initializeDetailedPerfConfig() {
  const char* reportEnv = std::getenv("NAJA_VRL_DUMPER_REPORT");
  if (!reportEnv) {
    return;
  }
  // LCOV_EXCL_START
  std::string reportPath(reportEnv);
  if (reportPath.empty() || reportPath == "1") {
    reportPath = "vrl_dumper_perf.log";
  }
  detailedPerfReport_.enabled = true;
  detailedPerfReport_.reportPath = reportPath;
  // LCOV_EXCL_STOP
}

bool SNLVRLDumper::beginDetailedPerfSession(const std::string& context) {
  if (!detailedPerfReport_.enabled || detailedPerfReport_.sessionActive) {
    return false;
  }
  // LCOV_EXCL_START
  auto reportPath = detailedPerfReport_.reportPath;
  detailedPerfReport_ = DetailedPerfReport {};
  detailedPerfReport_.enabled = true;
  detailedPerfReport_.sessionActive = true;
  detailedPerfReport_.reportPath = std::move(reportPath);
  detailedPerfReport_.context = context;
  detailedPerfReport_.sessionStart = std::chrono::steady_clock::now();
  return true;
  // LCOV_EXCL_STOP
}

// LCOV_EXCL_START
void SNLVRLDumper::finalizeDetailedPerfSession() {
  if (!detailedPerfReport_.enabled || !detailedPerfReport_.sessionActive) {
    return;
  }
  detailedPerfReport_.totalDuration =
    std::chrono::steady_clock::now() - detailedPerfReport_.sessionStart;
  detailedPerfReport_.sessionActive = false;

  std::ofstream output(detailedPerfReport_.reportPath, std::ios::out | std::ios::app);
  if (!output) {
    NAJA_LOG_WARN(
      "Unable to write Verilog dumper performance report: {}",
      detailedPerfReport_.reportPath.string());
    return;
  }

  output << "=== SNLVRLDumper Perf Report ===\n";
  output << "context=" << detailedPerfReport_.context << "\n";
  output << std::fixed << std::setprecision(3);
  output << "time.total_ms=" << toMilliseconds(detailedPerfReport_.totalDuration) << "\n";
  output << "time.dumpAttributes_ms="
         << toMilliseconds(detailedPerfReport_.dumpAttributesDuration) << "\n";
  output << "count.dumpAttributes_calls=" << detailedPerfReport_.dumpAttributesCalls << "\n";
  output << "count.dumpAttributes_empty_calls="
         << detailedPerfReport_.dumpAttributesEmptyCalls << "\n";
  output << "count.dumpAttributes_nonempty_calls="
         << detailedPerfReport_.dumpAttributesNonEmptyCalls << "\n";
  output << "count.dumped_attributes=" << detailedPerfReport_.dumpedAttributesCount << "\n";
  output << "time.dumpAttributes_snl_attributes_ms="
         << toMilliseconds(detailedPerfReport_.dumpAttributesSNLAttributesDuration) << "\n";
  output << "count.dumped_snl_attributes="
         << detailedPerfReport_.dumpedSNLAttributesCount << "\n";
  output << "time.dumpAttributes_rtl_infos_ms="
         << toMilliseconds(detailedPerfReport_.dumpAttributesRTLInfosDuration) << "\n";
  output << "count.dumped_rtl_infos="
         << detailedPerfReport_.dumpedRTLInfosCount << "\n";
  if (detailedPerfReport_.dumpedAttributesCount > 0) {
    const double dumpAttributesMicros =
      std::chrono::duration<double, std::micro>(
        detailedPerfReport_.dumpAttributesDuration).count();
    output << "derived.dumpAttributes_us_per_attribute="
           << (dumpAttributesMicros / detailedPerfReport_.dumpedAttributesCount) << "\n";
  }
  if (detailedPerfReport_.dumpedSNLAttributesCount > 0) {
    const double dumpSNLAttributesMicros =
      std::chrono::duration<double, std::micro>(
        detailedPerfReport_.dumpAttributesSNLAttributesDuration).count();
    output << "derived.dumpAttributes_snl_attributes_us_per_attribute="
           << (dumpSNLAttributesMicros / detailedPerfReport_.dumpedSNLAttributesCount) << "\n";
  }
  if (detailedPerfReport_.dumpedRTLInfosCount > 0) {
    const double dumpRTLInfosMicros =
      std::chrono::duration<double, std::micro>(
        detailedPerfReport_.dumpAttributesRTLInfosDuration).count();
    output << "derived.dumpAttributes_rtl_infos_us_per_attribute="
           << (dumpRTLInfosMicros / detailedPerfReport_.dumpedRTLInfosCount) << "\n";
  }
  output << "time.dumpAttributes_design_ms="
         << toMilliseconds(detailedPerfReport_.dumpAttributesDesignDuration) << "\n";
  output << "count.dumpAttributes_design_calls="
         << detailedPerfReport_.dumpAttributesDesignCalls << "\n";
  output << "count.dumped_attributes_design="
         << detailedPerfReport_.dumpedAttributesDesignCount << "\n";
  if (detailedPerfReport_.dumpedAttributesDesignCount > 0) {
    const double dumpAttributesDesignMicros =
      std::chrono::duration<double, std::micro>(
        detailedPerfReport_.dumpAttributesDesignDuration).count();
    output << "derived.dumpAttributes_design_us_per_attribute="
           << (dumpAttributesDesignMicros / detailedPerfReport_.dumpedAttributesDesignCount)
           << "\n";
  }
  output << "time.dumpAttributes_instance_ms="
         << toMilliseconds(detailedPerfReport_.dumpAttributesInstanceDuration) << "\n";
  output << "count.dumpAttributes_instance_calls="
         << detailedPerfReport_.dumpAttributesInstanceCalls << "\n";
  output << "count.dumped_attributes_instance="
         << detailedPerfReport_.dumpedAttributesInstanceCount << "\n";
  if (detailedPerfReport_.dumpedAttributesInstanceCount > 0) {
    const double dumpAttributesInstanceMicros =
      std::chrono::duration<double, std::micro>(
        detailedPerfReport_.dumpAttributesInstanceDuration).count();
    output << "derived.dumpAttributes_instance_us_per_attribute="
           << (dumpAttributesInstanceMicros / detailedPerfReport_.dumpedAttributesInstanceCount)
           << "\n";
  }
  output << "time.dumpAttributes_net_ms="
         << toMilliseconds(detailedPerfReport_.dumpAttributesNetDuration) << "\n";
  output << "count.dumpAttributes_net_calls="
         << detailedPerfReport_.dumpAttributesNetCalls << "\n";
  output << "count.dumped_attributes_net="
         << detailedPerfReport_.dumpedAttributesNetCount << "\n";
  if (detailedPerfReport_.dumpedAttributesNetCount > 0) {
    const double dumpAttributesNetMicros =
      std::chrono::duration<double, std::micro>(
        detailedPerfReport_.dumpAttributesNetDuration).count();
    output << "derived.dumpAttributes_net_us_per_attribute="
           << (dumpAttributesNetMicros / detailedPerfReport_.dumpedAttributesNetCount)
           << "\n";
  }
  output << "time.dumpInterface_ms="
         << toMilliseconds(detailedPerfReport_.dumpInterfaceDuration) << "\n";
  output << "count.dumpInterface_calls=" << detailedPerfReport_.dumpInterfaceCalls << "\n";
  output << "time.dumpNets_ms=" << toMilliseconds(detailedPerfReport_.dumpNetsDuration) << "\n";
  output << "count.dumpNets_calls=" << detailedPerfReport_.dumpNetsCalls << "\n";
  output << "time.dumpInstances_ms="
         << toMilliseconds(detailedPerfReport_.dumpInstancesDuration) << "\n";
  output << "count.dumpInstances_calls=" << detailedPerfReport_.dumpInstancesCalls << "\n";
  output << "time.dumpTermAssigns_ms="
         << toMilliseconds(detailedPerfReport_.dumpTermAssignsDuration) << "\n";
  output << "count.dumpTermAssigns_calls=" << detailedPerfReport_.dumpTermAssignsCalls << "\n";
  output << "time.dumpParameters_ms="
         << toMilliseconds(detailedPerfReport_.dumpParametersDuration) << "\n";
  output << "count.dumpParameters_calls=" << detailedPerfReport_.dumpParametersCalls << "\n";
  output << "time.dumpOneDesign_ms="
         << toMilliseconds(detailedPerfReport_.dumpOneDesignDuration) << "\n";
  output << "count.dumpOneDesign_calls=" << detailedPerfReport_.dumpOneDesignCalls << "\n";
  output << "time.dumpDesign_stream_ms="
         << toMilliseconds(detailedPerfReport_.dumpDesignStreamDuration) << "\n";
  output << "count.dumpDesign_stream_calls=" << detailedPerfReport_.dumpDesignStreamCalls << "\n";
  output << "time.dumpDesign_path_ms="
         << toMilliseconds(detailedPerfReport_.dumpDesignPathDuration) << "\n";
  output << "count.dumpDesign_path_calls=" << detailedPerfReport_.dumpDesignPathCalls << "\n";
  output << "time.dumpLibrary_stream_ms="
         << toMilliseconds(detailedPerfReport_.dumpLibraryStreamDuration) << "\n";
  output << "count.dumpLibrary_stream_calls=" << detailedPerfReport_.dumpLibraryStreamCalls
         << "\n";
  output << "time.dumpLibrary_path_ms="
         << toMilliseconds(detailedPerfReport_.dumpLibraryPathDuration) << "\n";
  output << "count.dumpLibrary_path_calls=" << detailedPerfReport_.dumpLibraryPathCalls << "\n";

  struct PhaseStat {
    const char* name;
    std::chrono::nanoseconds duration;
    size_t calls;
  };
  std::vector<PhaseStat> phases {
    {"dumpAttributes", detailedPerfReport_.dumpAttributesDuration,
      detailedPerfReport_.dumpAttributesCalls},
    {"dumpAttributes_snl_attributes", detailedPerfReport_.dumpAttributesSNLAttributesDuration,
      detailedPerfReport_.dumpAttributesCalls},
    {"dumpAttributes_rtl_infos", detailedPerfReport_.dumpAttributesRTLInfosDuration,
      detailedPerfReport_.dumpAttributesCalls},
    {"dumpInterface", detailedPerfReport_.dumpInterfaceDuration,
      detailedPerfReport_.dumpInterfaceCalls},
    {"dumpNets", detailedPerfReport_.dumpNetsDuration,
      detailedPerfReport_.dumpNetsCalls},
    {"dumpInstances", detailedPerfReport_.dumpInstancesDuration,
      detailedPerfReport_.dumpInstancesCalls},
    {"dumpTermAssigns", detailedPerfReport_.dumpTermAssignsDuration,
      detailedPerfReport_.dumpTermAssignsCalls},
    {"dumpParameters", detailedPerfReport_.dumpParametersDuration,
      detailedPerfReport_.dumpParametersCalls},
    {"dumpOneDesign", detailedPerfReport_.dumpOneDesignDuration,
      detailedPerfReport_.dumpOneDesignCalls},
    {"dumpDesign_stream", detailedPerfReport_.dumpDesignStreamDuration,
      detailedPerfReport_.dumpDesignStreamCalls},
    {"dumpDesign_path", detailedPerfReport_.dumpDesignPathDuration,
      detailedPerfReport_.dumpDesignPathCalls},
    {"dumpLibrary_stream", detailedPerfReport_.dumpLibraryStreamDuration,
      detailedPerfReport_.dumpLibraryStreamCalls},
    {"dumpLibrary_path", detailedPerfReport_.dumpLibraryPathDuration,
      detailedPerfReport_.dumpLibraryPathCalls},
  };
  std::sort(phases.begin(), phases.end(),
    [](const PhaseStat& lhs, const PhaseStat& rhs) {
      return lhs.duration > rhs.duration;
    });

  const double totalMs = toMilliseconds(detailedPerfReport_.totalDuration);
  output << "hotspots.top_by_time=\n";
  size_t rank = 1;
  for (const auto& phase: phases) {
    if (phase.duration.count() == 0 && phase.calls == 0) {
      continue;
    }
    const double phaseMs = toMilliseconds(phase.duration);
    const double pctTotal = totalMs > 0.0 ? (100.0 * phaseMs / totalMs) : 0.0;
    const double phaseMicros =
      std::chrono::duration<double, std::micro>(phase.duration).count();
    const double avgMicrosPerCall = phase.calls > 0
      ? (phaseMicros / phase.calls)
      : 0.0;
    output << "hotspot." << rank
           << "=" << phase.name
           << " time_ms=" << phaseMs
           << " pct_total=" << pctTotal
           << " calls=" << phase.calls
           << " avg_us_per_call=" << avgMicrosPerCall
           << "\n";
    ++rank;
  }
  output << "\n";
}
// LCOV_EXCL_STOP

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

void SNLVRLDumper::setDumpRTLInfosAsAttributes(bool mode) {
  configuration_.setDumpRTLInfosAsAttributes(mode);
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

std::string SNLVRLDumper::getBitNetString(
  const SNLBitNet* bitNet,
  const DesignInsideAnonymousNaming& naming) {
  if (!bitNet) {
    return "DUMMY";
  }
  if (bitNet->isAssign0()) {
    return "1'b0";
  }
  if (bitNet->isAssign1()) {
    return "1'b1";
  }
  if (auto scalarNet = dynamic_cast<const SNLScalarNet*>(bitNet)) {
    auto netName = getNetName(scalarNet, naming);
    return dumpName(netName.getString());
  }
  auto busNetBit = static_cast<const SNLBusNetBit*>(bitNet);
  auto bus = busNetBit->getBus();
  auto busName = getNetName(bus, naming);
  return dumpName(busName.getString()) + "[" + std::to_string(busNetBit->getBit()) + "]";
}

void SNLVRLDumper::dumpAttributes(
  const NLObject* object,
  std::ostream& o,
  AttributeDumpSite site) {
  DetailedPerfScopedTimer timer(
    detailedPerfReport_,
    detailedPerfReport_.dumpAttributesDuration,
    detailedPerfReport_.dumpAttributesCalls);
  const bool perfActive = detailedPerfReport_.enabled && detailedPerfReport_.sessionActive;
  std::chrono::steady_clock::time_point siteStart {};
  if (perfActive) {
    // LCOV_EXCL_START
    siteStart = std::chrono::steady_clock::now();
    switch (site) {
      case AttributeDumpSite::Design:
        ++detailedPerfReport_.dumpAttributesDesignCalls;
        break;
      case AttributeDumpSite::Instance:
        ++detailedPerfReport_.dumpAttributesInstanceCalls;
        break;
      case AttributeDumpSite::Net:
        ++detailedPerfReport_.dumpAttributesNetCalls;
        break;
      // LCOV_EXCL_STOP
    }
  }
  size_t dumpedSNLAttributes = 0;
  size_t dumpedRTLInfos = 0;
  std::chrono::steady_clock::time_point snlAttributesStart {};
  if (perfActive) {
    snlAttributesStart = std::chrono::steady_clock::now(); // LCOV_EXCL_LINE
  }
  for (const auto& attribute: SNLAttributes::getAttributes(object)) {
    ++dumpedSNLAttributes;
    o << "(* ";
    o << attribute.getName().getString();
    if (attribute.hasValue()) {
      o << "=";
      if (attribute.getValue().isString()) {
        o << "\"";
      }
      o << attribute.getValue().getString();
      if (attribute.getValue().isString()) {
        o << "\"";
      }
    }
    o << " *)\n";
  }
  if (perfActive) {
    // LCOV_EXCL_START
    detailedPerfReport_.dumpAttributesSNLAttributesDuration +=
      std::chrono::steady_clock::now() - snlAttributesStart;
    // LCOV_EXCL_STOP
  }
  if (configuration_.isDumpRTLInfosAsAttributes()) {
    const SNLRTLInfos* rtlInfos = nullptr;
    if (auto design = dynamic_cast<const SNLDesign*>(object)) {
      rtlInfos = design->getRTLInfos();
    } else if (auto designObject = dynamic_cast<const SNLDesignObject*>(object)) {
      rtlInfos = designObject->getRTLInfos();
    }
    std::chrono::steady_clock::time_point rtlInfosStart {};
    if (perfActive) {
      rtlInfosStart = std::chrono::steady_clock::now(); // LCOV_EXCL_LINE
    }
    if (rtlInfos) {
      for (const auto& info : rtlInfos->getInfos()) {
        ++dumpedRTLInfos;
        o << "(* ";
        o << info.first.getString();
        if (!info.second.empty()) {
          o << "=";
          const bool valueIsNumber = isUnsignedDecimal(info.second);
          if (!valueIsNumber) {
            o << "\"";
          }
          o << info.second;
          if (!valueIsNumber) {
            o << "\"";
          }
        }
        o << " *)\n";
      }
    }
    if (perfActive) {
      // LCOV_EXCL_START
      detailedPerfReport_.dumpAttributesRTLInfosDuration +=
        std::chrono::steady_clock::now() - rtlInfosStart;
      // LCOV_EXCL_STOP
    }
  }
  const size_t dumpedAttributes = dumpedSNLAttributes + dumpedRTLInfos;
  if (perfActive) {
    // LCOV_EXCL_START
    if (dumpedAttributes == 0) {
      ++detailedPerfReport_.dumpAttributesEmptyCalls;
    } else {
      ++detailedPerfReport_.dumpAttributesNonEmptyCalls;
    }
    detailedPerfReport_.dumpedAttributesCount += dumpedAttributes;
    detailedPerfReport_.dumpedSNLAttributesCount += dumpedSNLAttributes;
    detailedPerfReport_.dumpedRTLInfosCount += dumpedRTLInfos;
    const auto siteDuration = std::chrono::steady_clock::now() - siteStart;
    switch (site) {
      case AttributeDumpSite::Design:
        detailedPerfReport_.dumpAttributesDesignDuration += siteDuration;
        detailedPerfReport_.dumpedAttributesDesignCount += dumpedAttributes;
        break;
      case AttributeDumpSite::Instance:
        detailedPerfReport_.dumpAttributesInstanceDuration += siteDuration;
        detailedPerfReport_.dumpedAttributesInstanceCount += dumpedAttributes;
        break;
      case AttributeDumpSite::Net:
        detailedPerfReport_.dumpAttributesNetDuration += siteDuration;
        detailedPerfReport_.dumpedAttributesNetCount += dumpedAttributes;
        break;
    }
    // LCOV_EXCL_STOP
  }
}

void SNLVRLDumper::dumpInterface(const SNLDesign* design, std::ostream& o, DesignInsideAnonymousNaming& naming) {
  DetailedPerfScopedTimer timer(
    detailedPerfReport_,
    detailedPerfReport_.dumpInterfaceDuration,
    detailedPerfReport_.dumpInterfaceCalls);
  size_t nbChars = std::char_traits<char>::length("module  (");
  nbChars += design->getName().getString().size();
  o << "(";
  bool first = true;
  for (auto term: design->getTerms()) {
    if (not first) {
      o << ",";
      nbChars += 1;
      if (nbChars > 80) {
        o << '\n';
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
  dumpAttributes(net, o, AttributeDumpSite::Net);
  o << "wire ";
  if (auto bus = dynamic_cast<const SNLBusNet*>(net)) {
    o << "[" << bus->getMSB() << ":" << bus->getLSB() << "] ";
  }
  o << dumpName(netName.getString());
  o << ";" << '\n';
  return true;
}

void SNLVRLDumper::dumpNets(const SNLDesign* design, std::ostream& o, DesignInsideAnonymousNaming& naming) {
  DetailedPerfScopedTimer timer(
    detailedPerfReport_,
    detailedPerfReport_.dumpNetsDuration,
    detailedPerfReport_.dumpNetsCalls);
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
    o << '\n';
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
    auto dumpRangeWithNaming = [&](ContiguousNetBits& bits) {
      if (bits.empty()) {
        return;
      }
      if (bits[0]->isAssignConstant()) {
        dumpConstantRange(bits, firstElement, concatenation, connectionStr);
        return;
      }
      if (not firstElement) {
        connectionStr += ", ";
        concatenation = true;
      } else {
        firstElement = false;
      }
      auto* rangeMSBBit = static_cast<SNLBusNetBit*>(bits[0]);
      auto rangeMSB = rangeMSBBit->getBit();
      auto* rangeLSBBit = static_cast<SNLBusNetBit*>(bits[bits.size() - 1]);
      auto rangeLSB = rangeLSBBit->getBit();
      auto* bus = rangeMSBBit->getBus();
      auto busName = getNetName(bus, naming);
      auto busMSB = bus->getMSB();
      auto busLSB = bus->getLSB();
      if (rangeMSB == busMSB && rangeLSB == busLSB) {
        connectionStr += dumpName(busName.getString());
      } else if (rangeMSB == rangeLSB) {
        connectionStr += dumpName(busName.getString()) + "[";
        connectionStr += std::to_string(rangeMSB);
        connectionStr += "]";
      } else {
        connectionStr += dumpName(busName.getString()) + "[";
        connectionStr += std::to_string(rangeMSB);
        connectionStr += ":";
        connectionStr += std::to_string(rangeLSB);
        connectionStr += "]";
      }
      bits.clear();
    };
    for (auto net: termNets) {
      if (net) {
        if (net->isAssignConstant()) {
          if (not contiguousBits.empty()) {
            SNLBitNet* previousBit = contiguousBits.back();
            if (not previousBit->isAssignConstant()) {
              dumpRangeWithNaming(contiguousBits);
              contiguousBits = { net };
            } else {
              contiguousBits.push_back(net);
            }
          } else {
            contiguousBits.push_back(net);
          }
        } else if (dynamic_cast<SNLScalarNet*>(net)) {
          dumpRangeWithNaming(contiguousBits);
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
              dumpRangeWithNaming(contiguousBits);
              contiguousBits = { busNetBit };
            } else {
              SNLBusNetBit* previousBusBit = static_cast<SNLBusNetBit*>(previousBit);
              if (busNet == previousBusBit->getBus()
                and ((previousBusBit->getBit() == busNetBit->getBit()+1)
                or (previousBusBit->getBit() == busNetBit->getBit()-1))) {
                contiguousBits.push_back(busNetBit);
              } else {
                dumpRangeWithNaming(contiguousBits);
                contiguousBits = { busNetBit };
              }
            }
          } else {
            contiguousBits.push_back(busNetBit);
          }
        }
      } else {
        dumpRangeWithNaming(contiguousBits);
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
      dumpRangeWithNaming(contiguousBits);
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
          o << '\n';
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
      o << '\n';
      dumpInsTermConnectivity(previousTerm, termNets, o, naming);
    }
  }
  o << '\n' << ")";
}

void SNLVRLDumper::dumpInstParameters(
  const SNLInstance* instance,
  std::ostream& o) {
  std::vector<const SNLInstParameter*> dumpedParameters;
  for (auto instParameter: instance->getInstParameters()) {
    if (shouldDumpInstParameter(instance, instParameter)) {
      dumpedParameters.push_back(instParameter);
    }
  }
  std::sort(
    dumpedParameters.begin(),
    dumpedParameters.end(),
    [](const SNLInstParameter* lhs, const SNLInstParameter* rhs) {
      return lhs->getName().getString() < rhs->getName().getString();
    });
  if (not dumpedParameters.empty()) {
    bool first = true;
    o << "#(" << '\n';
    for (auto instParameter: dumpedParameters) {
      if (not first) {
        o << "," << '\n';
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
    o << '\n' << ") ";
  }
}

bool SNLVRLDumper::dumpInstance(
  const SNLInstance* instance,
  std::ostream& o,
  DesignInsideAnonymousNaming& naming) {
  if (NLDB0::isMux2(instance->getModel())) {
    emitNajaMux2Model_ = true;
    std::string instanceName;
    if (instance->isUnnamed()) {
      instanceName = createInstanceName(instance, naming);
    } else {
      instanceName = instance->getName().getString();
    }
    std::string widthValue = "1";
    if (auto* widthParam = instance->getModel()->getParameter(NLName("WIDTH"))) {
      widthValue = widthParam->getValue();
    }
    if (auto* widthInstParam = instance->getInstParameter(NLName("WIDTH"))) {
      widthValue = widthInstParam->getValue();
    }
    dumpAttributes(instance, o, AttributeDumpSite::Instance);
    o << "naja_mux2 ";
    if (widthValue != "1") {
      o << "#(" << '\n';
      o << "  .WIDTH(" << widthValue << ")" << '\n';
      o << ") ";
    }
    o << dumpName(instanceName);
    dumpInstanceInterface(instance, o, naming);
    o << ";" << '\n';
    return true;
  }
  if (NLDB0::isMemory(instance->getModel())) {
    emitNajaMemModel_ = true;
    std::string instanceName;
    if (instance->isUnnamed()) {
      instanceName = createInstanceName(instance, naming);
    } else {
      instanceName = instance->getName().getString();
    }
    dumpAttributes(instance, o, AttributeDumpSite::Instance);
    o << "naja_mem ";
    dumpInstParameters(instance, o);
    o << dumpName(instanceName);
    dumpInstanceInterface(instance, o, naming);
    o << ";" << '\n';
    return true;
  }
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
      o << getBitNetString(net, naming);
    }
    o << ");";
    o << '\n';
    return true;
  }
  assert(not NLDB0::isAssign(instance->getModel()));
  std::string instanceName;
  if (instance->isUnnamed()) {
    instanceName = createInstanceName(instance, naming);
  } else {
    instanceName = instance->getName().getString();
  }
  dumpAttributes(instance, o, AttributeDumpSite::Instance);
  auto model = instance->getModel();
  if (not model->isUnnamed()) { //FIXME !!
    o << dumpName(model->getName().getString()) << " ";
  }
  dumpInstParameters(instance, o);
  o << dumpName(instanceName);
  dumpInstanceInterface(instance, o, naming);
  o << ";" << '\n';
  return true;
}

void SNLVRLDumper::dumpInstances(const SNLDesign* design, std::ostream& o, DesignInsideAnonymousNaming& naming) {
  DetailedPerfScopedTimer timer(
    detailedPerfReport_,
    detailedPerfReport_.dumpInstancesDuration,
    detailedPerfReport_.dumpInstancesCalls);
  bool blankLine = false;
  auto bitNetToString = [&](const SNLBitNet* bitNet) {
    return getBitNetString(bitNet, naming);
  };
  auto busNetToString = [&](const SNLBusNet* busNet) {
    auto busName = getNetName(busNet, naming);
    return dumpName(busName.getString());
  };
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
          o << '\n';
        }
        blankLine = dumpSingleAssign(inputNet, outputNet, o, bitNetToString);
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
          o << '\n';
        }
        blankLine = dumpAssignGroup(group, o, bitNetToString, busNetToString);
      } else {
        if (blankLine) {
          o << '\n';
        }
        blankLine = dumpSingleAssign(inputNet, outputNet, o, bitNetToString);
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
      o << '\n';
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
        o << "assign " << netName << " = " << termNetName << ";" << '\n';
        break;
      case SNLTerm::Direction::Output:
        o << "assign " << termNetName << " = " << netName << ";" << '\n';
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
  DetailedPerfScopedTimer timer(
    detailedPerfReport_,
    detailedPerfReport_.dumpTermAssignsDuration,
    detailedPerfReport_.dumpTermAssignsCalls);
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
    o << '\n';
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
  o << " ;" << '\n';
}

void SNLVRLDumper::dumpParameters(const SNLDesign* design, std::ostream& o) {
  DetailedPerfScopedTimer timer(
    detailedPerfReport_,
    detailedPerfReport_.dumpParametersDuration,
    detailedPerfReport_.dumpParametersCalls);
  std::vector<const SNLParameter*> parameters;
  for (auto parameter: design->getParameters()) {
    parameters.push_back(parameter);
  }
  std::sort(
    parameters.begin(),
    parameters.end(),
    [](const SNLParameter* lhs, const SNLParameter* rhs) {
      return lhs->getName().getString() < rhs->getName().getString();
    });
  bool atLeastOne = false;
  for (auto parameter: parameters) {
    atLeastOne = true;
    dumpParameter(parameter, o);
  }
  if (atLeastOne) {
    o << '\n';
  }
}

void SNLVRLDumper::dumpOneDesign(const SNLDesign* design, std::ostream& o) {
  DetailedPerfScopedTimer timer(
    detailedPerfReport_,
    detailedPerfReport_.dumpOneDesignDuration,
    detailedPerfReport_.dumpOneDesignCalls);
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
  dumpAttributes(design, o, AttributeDumpSite::Design);
  o << "module " << dumpName(design->getName().getString());

  dumpInterface(design, o, naming);
  o << '\n';

  dumpParameters(design, o);
  dumpNets(design, o, naming);
  dumpTermAssigns(design, o);

  dumpInstances(design, o, naming);

  o << "endmodule //" << design->getName().getString();
  o << '\n';
}

void SNLVRLDumper::dumpNajaMux2Model(std::ostream& o) {
  o << "module naja_mux2 #(\n";
  o << "  parameter WIDTH = 1\n";
  o << ") (\n";
  o << "  input [WIDTH-1:0] A,\n";
  o << "  input [WIDTH-1:0] B,\n";
  o << "  input S,\n";
  o << "  output [WIDTH-1:0] Y\n";
  o << ");\n";
  o << "  assign Y = S ? B : A;\n";
  o << "endmodule //naja_mux2\n";
}

void SNLVRLDumper::dumpNajaMemModel(std::ostream& o) {
  o << "module naja_mem #(\n";
  o << "  parameter WIDTH = 1,\n";
  o << "  parameter DEPTH = 1,\n";
  o << "  parameter ABITS = 1,\n";
  o << "  parameter RD_PORTS = 1,\n";
  o << "  parameter WR_PORTS = 1,\n";
  o << "  parameter RST_ENABLE = 0,\n";
  o << "  parameter RST_ASYNC = 0,\n";
  o << "  parameter RST_ACTIVE_LOW = 0,\n";
  o << "  parameter INIT = 1'b0\n";
  o << ") (\n";
  o << "  input CLK,\n";
  o << "  input RST,\n";
  o << "  input [RD_PORTS*ABITS-1:0] RADDR,\n";
  o << "  output reg [RD_PORTS*WIDTH-1:0] RDATA,\n";
  o << "  input [WR_PORTS*ABITS-1:0] WADDR,\n";
  o << "  input [WR_PORTS*WIDTH-1:0] WDATA,\n";
  o << "  input [WR_PORTS-1:0] WE\n";
  o << ");\n\n";
  o << "  reg [WIDTH-1:0] mem [0:DEPTH-1];\n";
  o << "  integer i;\n";
  o << "  integer rp;\n";
  o << "  integer wp;\n";
  o << "  integer later;\n";
  o << "  reg allow_write;\n";
  o << "  reg [ABITS-1:0] addr_value;\n\n";
  o << "  task automatic load_init;\n";
  o << "    integer init_idx;\n";
  o << "    begin\n";
  o << "      for (init_idx = 0; init_idx < DEPTH; init_idx = init_idx + 1)\n";
  o << "        mem[init_idx] = INIT[init_idx*WIDTH +: WIDTH];\n";
  o << "    end\n";
  o << "  endtask\n\n";
  o << "  wire reset_active = RST_ENABLE && (RST_ACTIVE_LOW ? ~RST : RST);\n\n";
  o << "  always @* begin\n";
  o << "    for (rp = 0; rp < RD_PORTS; rp = rp + 1) begin\n";
  o << "      addr_value = RADDR[rp*ABITS +: ABITS];\n";
  o << "      if (addr_value < DEPTH)\n";
  o << "        RDATA[rp*WIDTH +: WIDTH] = mem[addr_value];\n";
  o << "      else\n";
  o << "        RDATA[rp*WIDTH +: WIDTH] = {WIDTH{1'b0}};\n";
  o << "    end\n";
  o << "  end\n\n";
  o << "  always @(posedge CLK";
  o << " or ";
  o << "posedge RST";
  o << " or ";
  o << "negedge RST";
  o << ") begin\n";
  o << "    if (RST_ASYNC && reset_active) begin\n";
  o << "      load_init();\n";
  o << "    end else begin\n";
  o << "      if (!RST_ASYNC && reset_active)\n";
  o << "        load_init();\n";
  o << "      else begin\n";
  o << "        for (wp = 0; wp < WR_PORTS; wp = wp + 1) begin\n";
  o << "          allow_write = WE[wp];\n";
  o << "          addr_value = WADDR[wp*ABITS +: ABITS];\n";
  o << "          for (later = wp + 1; later < WR_PORTS; later = later + 1) begin\n";
  o << "            if (WE[later] && WADDR[later*ABITS +: ABITS] == addr_value)\n";
  o << "              allow_write = 1'b0;\n";
  o << "          end\n";
  o << "          if (allow_write && addr_value < DEPTH)\n";
  o << "            mem[addr_value] <= WDATA[wp*WIDTH +: WIDTH];\n";
  o << "        end\n";
  o << "      end\n";
  o << "    end\n";
  o << "  end\n";
  o << "endmodule //naja_mem\n";
}

void SNLVRLDumper::dumpDesign(const SNLDesign* design, std::ostream& o) {
  std::string context("dumpDesign(stream): ");
  context += design->isUnnamed() ? "anonymous_design" : design->getName().getString();
  DetailedPerfSessionGuard sessionGuard(*this, context);
  DetailedPerfScopedTimer timer(
    detailedPerfReport_,
    detailedPerfReport_.dumpDesignStreamDuration,
    detailedPerfReport_.dumpDesignStreamCalls);
  emitNajaMemModel_ = false;
  emitNajaMux2Model_ = false;
  if (configuration_.isDumpHierarchy()) {
    SNLUtils::SortedDesigns designs;
    SNLUtils::getDesignsSortedByHierarchicalLevel(design, designs);
    bool first = true;
    for (auto designLevel: designs) {
      const SNLDesign* design = designLevel.first;
      if (not design->isPrimitive()) {
        if (not first) {
          o << '\n';
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
  std::string context("dumpLibrary(stream): ");
  context += library->isUnnamed() ? "anonymous_library" : library->getName().getString();
  DetailedPerfSessionGuard sessionGuard(*this, context);
  DetailedPerfScopedTimer timer(
    detailedPerfReport_,
    detailedPerfReport_.dumpLibraryStreamDuration,
    detailedPerfReport_.dumpLibraryStreamCalls);
  emitNajaMemModel_ = false;
  emitNajaMux2Model_ = false;
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

std::string SNLVRLDumper::getPrimitiveFileName() const {
  if (configuration_.hasLibraryFileName()) {
    return configuration_.getLibraryFileName();
  }
  return "primitives.v";
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

void SNLVRLDumper::dumpNajaPrimitiveFile(const std::filesystem::path& path) {
  if (!emitNajaMemModel_ && !emitNajaMux2Model_) {
    return;
  }
  std::filesystem::path filePath = path/getPrimitiveFileName();
  std::ofstream outFile;
  outFile.open(filePath);
  NajaUtils::createBanner(
    outFile,
    "Verilog file for naja primitives",
    "//"
  );
  outFile << '\n';
  bool needBlankLine = false;
  if (emitNajaMux2Model_) {
    dumpNajaMux2Model(outFile);
    needBlankLine = true;
  }
  if (emitNajaMemModel_) {
    if (needBlankLine) {
      outFile << '\n';
    }
    dumpNajaMemModel(outFile);
  }
}

void SNLVRLDumper::dumpDesign(const SNLDesign* design, const std::filesystem::path& path) {
  std::string context("dumpDesign(path): ");
  context += design->isUnnamed() ? "anonymous_design" : design->getName().getString();
  context += " -> ";
  context += path.string();
  DetailedPerfSessionGuard sessionGuard(*this, context);
  DetailedPerfScopedTimer timer(
    detailedPerfReport_,
    detailedPerfReport_.dumpDesignPathDuration,
    detailedPerfReport_.dumpDesignPathCalls);
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
    outFile << '\n';
    dumpDesign(design, outFile);
    if (emitNajaMemModel_ || emitNajaMux2Model_) {
      dumpNajaPrimitiveFile(path);
    }
  } else {
    SNLVRLDumper streamDumper;
    SNLVRLDumper::Configuration configuration(configuration_);
    configuration.setDumpHierarchy(false);
    streamDumper.setConfiguration(configuration);
    SNLUtils::SortedDesigns designs;
    SNLUtils::getDesignsSortedByHierarchicalLevel(design, designs);
    bool emitNajaMemModel = false;
    bool emitNajaMux2Model = false;
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
      outFile << '\n';
      streamDumper.dumpDesign(design, outFile);
      emitNajaMemModel = emitNajaMemModel or streamDumper.emitNajaMemModel_;
      emitNajaMux2Model = emitNajaMux2Model or streamDumper.emitNajaMux2Model_;
    }
    emitNajaMemModel_ = emitNajaMemModel;
    emitNajaMux2Model_ = emitNajaMux2Model;
    if (emitNajaMemModel || emitNajaMux2Model) {
      dumpNajaPrimitiveFile(path);
    }
  }
}

void SNLVRLDumper::dumpLibrary(const NLLibrary* library, const std::filesystem::path& path) {
  std::string context("dumpLibrary(path): ");
  context += library->isUnnamed() ? "anonymous_library" : library->getName().getString();
  context += " -> ";
  context += path.string();
  DetailedPerfSessionGuard sessionGuard(*this, context);
  DetailedPerfScopedTimer timer(
    detailedPerfReport_,
    detailedPerfReport_.dumpLibraryPathDuration,
    detailedPerfReport_.dumpLibraryPathCalls);
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
    outFile << '\n';
    dumpLibrary(library, outFile);
    if (emitNajaMemModel_ || emitNajaMux2Model_) {
      dumpNajaPrimitiveFile(path);
    }
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
