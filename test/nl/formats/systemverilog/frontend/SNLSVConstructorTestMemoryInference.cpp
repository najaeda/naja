// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <sstream>
#include <unordered_set>

#include "DNL.h"
#include "NLUniverse.h"
#include "NLDB0.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLBitNet.h"
#include "SNLInstParameter.h"
#include "SNLInstTerm.h"
#include "SNLInstance.h"
#include "SNLNet.h"
#include "SNLDesign.h"
#include "SNLDesignModeling.h"
#include "SNLDesignObject.h"
#include "SNLRTLInfos.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "SNLTerm.h"
#include "SNLUtils.h"
#include "SNLVRLDumper.h"

#include "SNLSVConstructor.h"
#include "SNLSVConstructorTestDetail.h"
#include "SNLSVConstructorException.h"

using namespace naja::NL;

#ifndef SNL_SV_BENCHMARKS_PATH
#define SNL_SV_BENCHMARKS_PATH "Undefined"
#endif
#ifndef SNL_SV_DUMPER_TEST_PATH
#define SNL_SV_DUMPER_TEST_PATH "Undefined"
#endif

namespace {

const SNLRTLInfos* getRTLInfos(const NLObject* object) {
  if (auto design = dynamic_cast<const SNLDesign*>(object)) {
    return design->getRTLInfos();
  }
  if (auto designObject = dynamic_cast<const SNLDesignObject*>(object)) {
    return designObject->getRTLInfos();
  }
  return nullptr;
}

bool hasRTLInfo(const NLObject* object, const std::string& name) {
  auto rtlInfos = getRTLInfos(object);
  return rtlInfos && rtlInfos->hasInfo(NLName(name));
}

std::string readTextFile(const std::filesystem::path& path) {
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

std::filesystem::path dumpTopAndGetVerilogPath(const SNLDesign* top,
                                               const std::string& outDirName,
                                               bool dumpRTLInfosAsAttributes = false) {
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= outDirName;
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  SNLVRLDumper dumper;
  auto fileName = top->getName().getString() + ".v";
  dumper.setTopFileName(fileName);
  dumper.setSingleFile(true);
  dumper.setDumpRTLInfosAsAttributes(dumpRTLInfosAsAttributes);
  dumper.dumpDesign(top, outPath);
  return outPath / fileName;
}

size_t getMux2Width(const SNLInstance* instance) {
  if (!instance || !NLDB0::isMux2(instance->getModel())) {
    return 0;
  }
  if (auto* widthInstParam = instance->getInstParameter(NLName("WIDTH"))) {
    return static_cast<size_t>(std::stoull(widthInstParam->getValue()));
  }
  auto* widthParam = instance->getModel()->getParameter(NLName("WIDTH"));
  if (!widthParam) {
    return 0;
  }
  return static_cast<size_t>(std::stoull(widthParam->getValue()));
}

size_t countMux2Instances(const SNLDesign* design, size_t width = 0) {
  size_t count = 0;
  for (auto inst : design->getInstances()) {
    if (!NLDB0::isMux2(inst->getModel())) {
      continue;
    }
    if (width != 0 && getMux2Width(inst) != width) {
      continue;
    }
    ++count;
  }
  return count;
}

size_t countFAInstances(const SNLDesign* design) {
  size_t count = 0;
  for (auto inst : design->getInstances()) {
    if (NLDB0::isFA(inst->getModel())) {
      ++count;
    }
  }
  return count;
}

size_t countAssignDrivers(const SNLDesign* design, const SNLBitNet* net) {
  size_t count = 0;
  auto assignOutput = NLDB0::getAssignOutput();
  for (auto inst : design->getInstances()) {
    if (!NLDB0::isAssign(inst->getModel())) {
      continue;
    }
    auto instTerm = inst->getInstTerm(assignOutput);
    if (instTerm && instTerm->getNet() == net) {
      ++count;
    }
  }
  return count;
}

std::vector<std::string> collectDanglingInternalInputTerms(SNLDesign* top) {
  auto* universe = NLUniverse::get();
  if (universe == nullptr) {
    return {};
  }
  universe->setTopDesign(top);
  naja::DNL::destroy();
  auto* dnl = naja::DNL::get();
  std::vector<std::string> offenders;
  for (naja::DNL::DNLID termID = 0; termID < dnl->getDNLTerms().size(); ++termID) {
    const auto& term = dnl->getDNLTerminalFromID(termID);
    if (term.isNull() || term.isTopPort()) {
      continue;
    }
    auto* bitTerm = term.getSnlBitTerm();
    if (bitTerm == nullptr || bitTerm->getDirection() != SNLTerm::Direction::Input) {
      continue;
    }
    const auto isoID = term.getIsoID();
    if (isoID == naja::DNL::DNLID_MAX) {
      continue;
    }
    const auto& iso = dnl->getDNLIsoDB().getIsoFromIsoIDconst(isoID);
    if (iso.isConstant() || !iso.getDrivers().empty()) {
      continue;
    }

    std::ostringstream message;
    message << term.getDNLInstance().getFullPath() << "."
            << bitTerm->getName().getString() << "[" << bitTerm->getBit() << "]"
            << " iso=" << isoID
            << " readers=" << iso.getReaders().size()
            << " model=" << term.getDNLInstance().getSNLModel()->getName().getString();
    if (auto* snlInstance = term.getDNLInstance().getSNLInstance()) {
      if (snlInstance->hasRTLInfos()) {
        auto* rtlInfos = snlInstance->getRTLInfos();
        if (rtlInfos->hasInfo(NLName("sv_src_line"))) {
          message << " src_line=" << rtlInfos->getInfo(NLName("sv_src_line"));
        }
        if (rtlInfos->hasInfo(NLName("sv_src_file"))) {
          message << " src_file=" << rtlInfos->getInfo(NLName("sv_src_file"));
        }
      }
      if (auto* instTerm = snlInstance->getInstTerm(bitTerm)) {
        if (auto* net = instTerm->getNet()) {
          message << " net=" << net->getName().getString();
        }
      }
    }
    offenders.push_back(message.str());
  }
  naja::DNL::destroy();
  return offenders;
}

std::vector<std::string> collectUndrivenMemoryReadOutputs(SNLDesign* top) {
  auto* universe = NLUniverse::get();
  if (universe == nullptr) {
    return {};
  }
  universe->setTopDesign(top);
  naja::DNL::destroy();
  auto* dnl = naja::DNL::get();
  std::vector<std::string> offenders;
  for (auto* inst : top->getInstances()) {
    if (!NLDB0::isMemory(inst->getModel())) {
      continue;
    }
    auto* rdata = NLDB0::getMemoryReadData(inst->getModel());
    if (!rdata) {
      continue;
    }
    for (auto* bitTerm : rdata->getBits()) {
      auto* instTerm = inst->getInstTerm(bitTerm);
      if (!instTerm) {
        continue;
      }
      const auto termID = dnl->getTop().getChildInstance(inst).getTerminalFromBitTerm(bitTerm).getID();
      if (termID == naja::DNL::DNLID_MAX) {
        continue;
      }
      const auto& term = dnl->getDNLTerminalFromID(termID);
      const auto isoID = term.getIsoID();
      if (isoID == naja::DNL::DNLID_MAX) {
        continue;
      }
      const auto& iso = dnl->getDNLIsoDB().getIsoFromIsoIDconst(isoID);
      if (!iso.getDrivers().empty()) {
        continue;
      }
      std::ostringstream message;
      message << inst->getName().getString() << "."
              << bitTerm->getName().getString() << "[" << bitTerm->getBit() << "]"
              << " iso=" << isoID
              << " readers=" << iso.getReaders().size();
      offenders.push_back(message.str());
    }
  }
  naja::DNL::destroy();
  return offenders;
}

std::string formatDNLTerm(const naja::DNL::DNLTerminalFull& term) {
  std::ostringstream message;
  if (term.isTopPort()) {
    message << term.getSnlBitTerm()->getDesign()->getName().getString() << ":"
            << term.getSnlBitTerm()->getName().getString() << "["
            << term.getSnlBitTerm()->getBit() << "]";
    return message.str();
  }
  message << term.getDNLInstance().getFullPath() << "."
          << term.getSnlBitTerm()->getName().getString() << "["
          << term.getSnlBitTerm()->getBit() << "]";
  return message.str();
}

std::vector<naja::DNL::DNLID> collectCombinationalInputTermIDs(
    const naja::DNL::DNLTerminalFull& driverTerm) {
  std::vector<naja::DNL::DNLID> termIDs;
  if (driverTerm.isNull() || driverTerm.isTopPort()) {
    return termIDs;
  }
  const auto combinationalInputs =
      SNLDesignModeling::getCombinatorialInputs(driverTerm.getSnlBitTerm());
  const auto& driverInstance = driverTerm.getDNLInstance();
  for (auto* inputBitTerm : combinationalInputs) {
    if (inputBitTerm == nullptr ||
        inputBitTerm->getDirection() == SNLTerm::Direction::Output) {
      continue;
    }
    const auto& inputTerm = driverInstance.getTerminalFromBitTerm(inputBitTerm);
    if (inputTerm.isNull()) {
      continue;
    }
    termIDs.push_back(inputTerm.getID());
  }
  std::sort(termIDs.begin(), termIDs.end());
  termIDs.erase(std::unique(termIDs.begin(), termIDs.end()), termIDs.end());
  return termIDs;
}

bool collectMemoryBoundaryConeIssue(
    decltype(naja::DNL::get()) dnl,
    naja::DNL::DNLID currentTermID,
    std::unordered_set<naja::DNL::DNLID>& activeTerms,
    std::unordered_set<naja::DNL::DNLID>& provenTerms,
    std::vector<std::string>& path,
    std::string& issue) {
  if (currentTermID == naja::DNL::DNLID_MAX) {
    issue = "encountered an invalid DNL term while tracing a memory dependency";
    return true;
  }
  if (provenTerms.find(currentTermID) != provenTerms.end()) {
    return false;
  }
  if (!activeTerms.insert(currentTermID).second) {
    std::ostringstream message;
    message << "logical loop at " << formatDNLTerm(dnl->getDNLTerminalFromID(currentTermID));
    issue = message.str();
    return true;
  }
  const auto& term = dnl->getDNLTerminalFromID(currentTermID);
  path.push_back(formatDNLTerm(term));

  if (term.isTopPort() || term.isSequential()) {
    provenTerms.insert(currentTermID);
    path.pop_back();
    activeTerms.erase(currentTermID);
    return false;
  }

  const auto isoID = term.getIsoID();
  if (isoID == naja::DNL::DNLID_MAX) {
    issue = "disconnected term " + formatDNLTerm(term);
    path.pop_back();
    activeTerms.erase(currentTermID);
    return true;
  }
  const auto& iso = dnl->getDNLIsoDB().getIsoFromIsoIDconst(isoID);
  if (iso.isConstant()) {
    provenTerms.insert(currentTermID);
    path.pop_back();
    activeTerms.erase(currentTermID);
    return false;
  }
  if (iso.getDrivers().empty()) {
    issue = "undriven term " + formatDNLTerm(term);
    path.pop_back();
    activeTerms.erase(currentTermID);
    return true;
  }
  if (iso.getDrivers().size() > 1) {
    std::ostringstream message;
    message << "multi-driver term " << formatDNLTerm(term);
    issue = message.str();
    path.pop_back();
    activeTerms.erase(currentTermID);
    return true;
  }

  const auto& driverTerm = dnl->getDNLTerminalFromID(iso.getDrivers().front());
  if (driverTerm.isTopPort() || driverTerm.isSequential()) {
    provenTerms.insert(currentTermID);
    path.pop_back();
    activeTerms.erase(currentTermID);
    return false;
  }
  const auto inputTermIDs = collectCombinationalInputTermIDs(driverTerm);
  if (inputTermIDs.empty()) {
    issue = "no combinational support terms for driver " + formatDNLTerm(driverTerm);
    path.pop_back();
    activeTerms.erase(currentTermID);
    return true;
  }

  for (const auto inputTermID : inputTermIDs) {
    if (collectMemoryBoundaryConeIssue(
            dnl, inputTermID, activeTerms, provenTerms, path, issue)) {
      path.pop_back();
      activeTerms.erase(currentTermID);
      return true;
    }
  }
  provenTerms.insert(currentTermID);
  path.pop_back();
  activeTerms.erase(currentTermID);
  return false;
}

std::optional<naja::DNL::DNLID> resolveTransparentDriverTarget(
    decltype(naja::DNL::get()) dnl,
    naja::DNL::DNLID termID) {
  std::unordered_set<naja::DNL::DNLID> visitedTerms;
  naja::DNL::DNLID currentTermID = termID;
  while (currentTermID != naja::DNL::DNLID_MAX &&
         visitedTerms.insert(currentTermID).second) {
    const auto& currentTerm = dnl->getDNLTerminalFromID(currentTermID);
    if (currentTerm.isNull()) {
      return std::nullopt;
    }

    if (currentTerm.getSnlBitTerm()->getDirection() != SNLTerm::Direction::Output) {
      const auto isoID = currentTerm.getIsoID();
      if (isoID == naja::DNL::DNLID_MAX) {
        return std::nullopt;
      }
      const auto& iso = dnl->getDNLIsoDB().getIsoFromIsoIDconst(isoID);
      if (iso.isConstant() || iso.getDrivers().size() != 1) {
        return std::nullopt;
      }
      currentTermID = iso.getDrivers().front();
      continue;
    }

    auto* model = currentTerm.getDNLInstance().getSNLModel();
    if (model == nullptr || !NLDB0::isAssign(model)) {
      return currentTermID;
    }

    auto* inputBitTerm = NLDB0::getAssignInput();
    if (inputBitTerm == nullptr) {
      return std::nullopt;
    }
    const auto& inputTerm =
        currentTerm.getDNLInstance().getTerminalFromBitTerm(inputBitTerm);
    if (inputTerm.isNull()) {
      return std::nullopt;
    }
    currentTermID = inputTerm.getID();
  }
  return std::nullopt;
}

std::vector<std::string> collectTransparentCombinationalSelfReferences(
    SNLDesign* top,
    const std::vector<std::string>& outputNameFilters = {}) {
  auto* universe = NLUniverse::get();
  if (universe == nullptr) {
    return {};
  }
  universe->setTopDesign(top);
  naja::DNL::destroy();
  auto* dnl = naja::DNL::get();

  std::vector<std::string> offenders;
  for (naja::DNL::DNLID termID = 0; termID < dnl->getDNLTerms().size(); ++termID) {
    const auto& term = dnl->getDNLTerminalFromID(termID);
    if (term.isNull() || term.isTopPort()) {
      continue;
    }
    auto* bitTerm = term.getSnlBitTerm();
    if (bitTerm == nullptr || bitTerm->getDirection() != SNLTerm::Direction::Output) {
      continue;
    }
    const auto outputName = formatDNLTerm(term);
    if (!outputNameFilters.empty()) {
      bool matchesFilter = false;
      for (const auto& filter : outputNameFilters) {
        if (outputName.find(filter) != std::string::npos) {
          matchesFilter = true;
          break;
        }
      }
      if (!matchesFilter) {
        continue;
      }
    }

    const auto inputTermIDs = collectCombinationalInputTermIDs(term);
    if (inputTermIDs.empty()) {
      continue;
    }
    for (const auto inputTermID : inputTermIDs) {
      const auto resolvedDriver = resolveTransparentDriverTarget(dnl, inputTermID);
      if (!resolvedDriver.has_value() || *resolvedDriver != termID) {
        continue;
      }

      std::ostringstream message;
      message << outputName << " depends on transparent alias "
              << formatDNLTerm(dnl->getDNLTerminalFromID(inputTermID))
              << " that resolves back to the same output";
      offenders.push_back(message.str());
    }
  }

  naja::DNL::destroy();
  return offenders;
}

std::vector<std::string> collectUnsupportedMemoryBoundaryTerms(SNLDesign* top) {
  auto* universe = NLUniverse::get();
  if (universe == nullptr) {
    return {};
  }
  universe->setTopDesign(top);
  naja::DNL::destroy();
  auto* dnl = naja::DNL::get();
  std::vector<std::string> issues;
  std::unordered_set<naja::DNL::DNLID> provenTerms;

  auto addIssueForBitTerms = [&](SNLInstance* inst,
                                 const SNLDesignModeling::BitTerms& bitTerms,
                                 const char* role) {
    for (auto* bitTerm : bitTerms) {
      if (bitTerm == nullptr) {
        continue;
      }
      auto* instTerm = inst->getInstTerm(bitTerm);
      if (instTerm == nullptr) {
        continue;
      }
      const auto& dnlTerm =
          dnl->getTop().getChildInstance(inst).getTerminal(instTerm);
      if (dnlTerm.isNull()) {
        continue;
      }

      std::unordered_set<naja::DNL::DNLID> activeTerms;
      std::vector<std::string> path;
      std::string issue;
      if (!collectMemoryBoundaryConeIssue(
              dnl,
              dnlTerm.getID(),
              activeTerms,
              provenTerms,
              path,
              issue)) {
        continue;
      }

      std::ostringstream message;
      // Guard SEC memory-model regressions directly at the inferred-memory
      // boundary surface: every exposed dependency bit must have a supported
      // combinational cone so later PDR/SEC extraction does not have to skip
      // the memory term as opaque or broken.
      message << inst->getName().getString() << "."
              << bitTerm->getName().getString() << "[" << bitTerm->getBit() << "]"
              << " role=" << role << " issue=" << issue;
      if (!path.empty()) {
        message << " path=";
        for (size_t i = 0; i < path.size(); ++i) {
          if (i != 0) {
            message << " -> ";
          }
          message << path[i];
        }
      }
      issues.push_back(message.str());
    }
  };

  for (auto* inst : top->getInstances()) {
    if (!NLDB0::isMemory(inst->getModel()) ||
        !SNLDesignModeling::hasMemoryInterface(inst->getModel())) {
      continue;
    }
    const auto interface = SNLDesignModeling::getMemoryInterface(inst);
    for (const auto& readPort : interface.readPorts) {
      addIssueForBitTerms(inst, readPort.address, "read_address");
      addIssueForBitTerms(inst, readPort.data, "read_data");
      addIssueForBitTerms(inst, readPort.enables, "read_enable");
    }
    for (const auto& writePort : interface.writePorts) {
      addIssueForBitTerms(inst, writePort.address, "write_address");
      addIssueForBitTerms(inst, writePort.data, "write_data");
      addIssueForBitTerms(inst, writePort.mask, "write_mask");
      addIssueForBitTerms(inst, writePort.enables, "write_enable");
      for (const auto& extraInputs : writePort.extraWriteInputs) {
        addIssueForBitTerms(inst, extraInputs, "extra_write_input");
      }
    }
  }
  naja::DNL::destroy();
  return issues;
}

std::string formatStringVector(const std::vector<std::string>& values) {
  std::ostringstream stream;
  for (size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      stream << "\n";
    }
    stream << values[i];
  }
  return stream.str();
}

struct Cva6SourceContext {
  std::filesystem::path cva6RepoDir;
  std::filesystem::path hpdcacheDir;
  std::string targetCfg;
};

std::optional<Cva6SourceContext> resolveCva6SourceContext() {
  const auto* cva6RepoDirEnv = std::getenv("CVA6_REPO_DIR");
  const auto* hpdcacheDirEnv = std::getenv("HPDCACHE_DIR");
  const auto* targetCfgEnv = std::getenv("TARGET_CFG");

  std::filesystem::path cva6RepoDir;
  if (cva6RepoDirEnv != nullptr && *cva6RepoDirEnv != '\0') {
    cva6RepoDir = cva6RepoDirEnv;
  } else {
    const std::filesystem::path fallbackRepoDir("/Users/noamcohen/dev/CVA6/cva6");
    if (std::filesystem::exists(fallbackRepoDir)) {
      cva6RepoDir = fallbackRepoDir;
    }
  }

  if (cva6RepoDir.empty() || !std::filesystem::exists(cva6RepoDir)) {
    return std::nullopt;
  }

  std::filesystem::path hpdcacheDir;
  if (hpdcacheDirEnv != nullptr && *hpdcacheDirEnv != '\0') {
    hpdcacheDir = hpdcacheDirEnv;
  } else {
    const auto fallbackHpdcacheDir =
        cva6RepoDir / "core" / "cache_subsystem" / "hpdcache";
    if (std::filesystem::exists(fallbackHpdcacheDir)) {
      hpdcacheDir = fallbackHpdcacheDir;
    }
  }

  if (hpdcacheDir.empty() || !std::filesystem::exists(hpdcacheDir)) {
    return std::nullopt;
  }

  std::string targetCfg = "cv64a6_imafdc_sv39";
  if (targetCfgEnv != nullptr && *targetCfgEnv != '\0') {
    targetCfg = targetCfgEnv;
  }

  return Cva6SourceContext{
      std::move(cva6RepoDir), std::move(hpdcacheDir), std::move(targetCfg)};
}

std::string substituteFlistVariables(
    std::string text,
    const Cva6SourceContext& context) {
  const std::array<std::pair<std::string_view, std::string>, 3> substitutions{{
      {"${CVA6_REPO_DIR}", context.cva6RepoDir.string()},
      {"${HPDCACHE_DIR}", context.hpdcacheDir.string()},
      {"${TARGET_CFG}", context.targetCfg},
  }};

  for (const auto& [needle, replacement] : substitutions) {
    size_t pos = 0;
    while ((pos = text.find(needle, pos)) != std::string::npos) {
      text.replace(pos, needle.size(), replacement);
      pos += replacement.size();
    }
  }
  return text;
}

void collectExpandedSlangArgsFromCommandFile(
    const std::filesystem::path& commandFile,
    const Cva6SourceContext& context,
    std::unordered_set<std::string>& visitedFiles,
    std::vector<std::string>& args) {
  const auto normalizedPath = std::filesystem::weakly_canonical(commandFile);
  if (!visitedFiles.insert(normalizedPath.string()).second) {
    return;
  }

  std::ifstream input(normalizedPath);
  ASSERT_TRUE(input.good()) << "Failed to read command file: " << normalizedPath.string();

  std::string line;
  while (std::getline(input, line)) {
    line = substituteFlistVariables(line, context);
    const auto commentPos = line.find("//");
    if (commentPos != std::string::npos) {
      line.erase(commentPos);
    }
    const auto first = line.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
      continue;
    }
    const auto last = line.find_last_not_of(" \t\r\n");
    line = line.substr(first, last - first + 1);
    if (line.empty()) {
      continue;
    }

    if (line.rfind("-F ", 0) == 0 || line.rfind("-f ", 0) == 0) {
      const auto nestedPath =
          normalizedPath.parent_path() / line.substr(3);
      collectExpandedSlangArgsFromCommandFile(
          nestedPath, context, visitedFiles, args);
      continue;
    }
    args.push_back(std::move(line));
  }
}

SNLSVConstructor::Paths buildExpandedCva6SlangArgs(
    const Cva6SourceContext& context,
    const std::string& topName,
    const std::vector<std::filesystem::path>& extraSources = {}) {
  const auto flistPath = context.cva6RepoDir / "core" / "Flist.cva6";
  std::unordered_set<std::string> visitedFiles;
  std::vector<std::string> args;
  collectExpandedSlangArgsFromCommandFile(
      flistPath, context, visitedFiles, args);
  for (const auto& extraSource : extraSources) {
    args.push_back(extraSource.string());
  }
  args.push_back("--top");
  args.push_back(topName);

  SNLSVConstructor::Paths paths;
  paths.reserve(args.size());
  for (const auto& arg : args) {
    paths.emplace_back(arg);
  }
  return paths;
}

void expectUnsupportedConstruct(
  SNLSVConstructor& constructor,
  const std::filesystem::path& svPath,
  std::initializer_list<const char*> expectedSubstrings) {
  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported SystemVerilog element exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported SystemVerilog elements encountered"))
      << reason;
    for (const auto* expected : expectedSubstrings) {
      EXPECT_NE(std::string::npos, reason.find(expected)) << reason;
    }
  }
}

}

class SNLSVConstructorTestMemoryInference: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      library_ = NLLibrary::create(db, NLName("SVLIB"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
      library_ = nullptr;
    }
  protected:
    NLLibrary* library_ {nullptr};
};

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  size_t mux2Count = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
    if (NLDB0::isMux2(inst->getModel())) {
      ++mux2Count;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
  EXPECT_EQ(0u, mux2Count);

  auto* model = memoryInst->getModel();
  ASSERT_NE(nullptr, model);
  EXPECT_TRUE(NLDB0::isMemory(model));
  EXPECT_EQ("4", model->getParameter(NLName("DEPTH"))->getValue());
  EXPECT_EQ("8", model->getParameter(NLName("WIDTH"))->getValue());

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  auto* depthParam = memoryInst->getInstParameter(NLName("DEPTH"));
  auto* abitsParam = memoryInst->getInstParameter(NLName("ABITS"));
  auto* rdPortsParam = memoryInst->getInstParameter(NLName("RD_PORTS"));
  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  ASSERT_NE(nullptr, widthParam);
  ASSERT_NE(nullptr, depthParam);
  ASSERT_NE(nullptr, abitsParam);
  ASSERT_NE(nullptr, rdPortsParam);
  ASSERT_NE(nullptr, wrPortsParam);
  ASSERT_NE(nullptr, rstEnableParam);
  EXPECT_EQ("8", widthParam->getValue());
  EXPECT_EQ("4", depthParam->getValue());
  EXPECT_EQ("2", abitsParam->getValue());
  EXPECT_EQ("1", rdPortsParam->getValue());
  EXPECT_EQ("1", wrPortsParam->getValue());
  EXPECT_EQ("0", rstEnableParam->getValue());

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "qd_memory_inference_supported");
  std::string dumpedText = readTextFile(dumpedVerilog);
  EXPECT_NE(std::string::npos, dumpedText.find("naja_mem #("));
  EXPECT_NE(std::string::npos, dumpedText.find(".WIDTH(8)"));
  EXPECT_NE(std::string::npos, dumpedText.find(".DEPTH(4)"));
  EXPECT_EQ(std::string::npos, dumpedText.find("module naja_mem #("));
  EXPECT_EQ(std::string::npos, dumpedText.find("naja_mux2"));
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSharedSequentialKeepsScalarFlops) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_keeps_scalar_flops";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_keeps_scalar_flops.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_keeps_scalar_flops(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       inc_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic [1:0] ptr_o,
  output logic [2:0] cnt_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [1:0] ptr_q;
  logic [1:0] ptr_n;
  logic [2:0] cnt_q;
  logic [2:0] cnt_n;

  always_comb begin
    mem_d = mem_q;
    ptr_n = ptr_q;
    cnt_n = cnt_q;
    if (inc_i) begin
      mem_d[addr_i] = data_i;
      ptr_n = ptr_q + 1'b1;
      cnt_n = cnt_q + 1'b1;
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      mem_q <= '{default: '0};
      ptr_q <= '0;
      cnt_q <= '0;
    end else begin
      mem_q <= mem_d;
      ptr_q <= ptr_n;
      cnt_q <= cnt_n;
    end
  end

  assign data_o = mem_q[addr_i];
  assign ptr_o = ptr_q;
  assign cnt_o = cnt_q;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_keeps_scalar_flops"));
  ASSERT_NE(top, nullptr);

  auto dffrnModel = NLDB0::getDFFRN();
  ASSERT_NE(dffrnModel, nullptr);
  size_t memoryCount = 0;
  size_t dffrnCount = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ++memoryCount;
    }
    if (inst->getModel() == dffrnModel) {
      ++dffrnCount;
    }
  }
  EXPECT_EQ(1u, memoryCount);
  EXPECT_EQ(5u, dffrnCount);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceWriteOnlySupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_write_only_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_write_only_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_write_only_supported(
  input  logic       clk_i,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic       flag_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    flag_o = en_i;
    mem_d = mem_q;
    if (en_i) begin
      mem_d[addr_i] = data_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_write_only_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("flag_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rdPortsParam = memoryInst->getInstParameter(NLName("RD_PORTS"));
  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, rdPortsParam);
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", rdPortsParam->getValue());
  EXPECT_EQ("1", wrPortsParam->getValue());

  EXPECT_NE(nullptr, top->getNet(NLName("mem_q_mem_dummy_raddr")));
  EXPECT_NE(nullptr, top->getNet(NLName("mem_q_mem_dummy_rdata")));
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceNonBitstreamElementFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_non_bitstream_element_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_non_bitstream_element_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_non_bitstream_element_fallback(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  typedef struct {
    logic [7:0] data;
    string      tag;
  } entry_t;

  entry_t mem_q [0:3];
  entry_t mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      mem_d[addr_i].data = data_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i].data;
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceResetInitSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_reset_init_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      mem_q <= '{8'h00, 8'h11, 8'h22, 8'h33};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_reset_init_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("1", rstAsyncParam->getValue());
  EXPECT_EQ("1", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b00110011001000100001000100000000", initParam->getValue());

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "qd_memory_inference_reset_init_supported");
  std::string dumpedText = readTextFile(dumpedVerilog);
  EXPECT_NE(std::string::npos, dumpedText.find("naja_mem #("));
  EXPECT_EQ(std::string::npos, dumpedText.find("module naja_mem #("));
  EXPECT_NE(std::string::npos, dumpedText.find(".RST_ENABLE(1)"));
  EXPECT_NE(std::string::npos, dumpedText.find(".RST_ASYNC(1)"));
  EXPECT_NE(std::string::npos, dumpedText.find(".RST_ACTIVE_LOW(1)"));
  EXPECT_NE(
    std::string::npos,
    dumpedText.find(".INIT(32'b00110011001000100001000100000000)"));

  const auto primitiveDumpPath = dumpedVerilog.parent_path() / "naja_primitives.v";
  ASSERT_TRUE(std::filesystem::exists(primitiveDumpPath));
  std::string primitiveDump = readTextFile(primitiveDumpPath);
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_mem #("));
  EXPECT_NE(std::string::npos, primitiveDump.find("reg [WIDTH-1:0] mem [0:DEPTH-1];"));
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceAsyncHighResetInitSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_async_high_reset_init_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_async_high_reset_init_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_async_high_reset_init_supported(
  input  logic       clk_i,
  input  logic       rst_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or posedge rst_i) begin
    if (rst_i) begin
      mem_q <= '{8'hA0, 8'hB1, 8'hC2, 8'hD3};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_async_high_reset_init_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("1", rstAsyncParam->getValue());
  EXPECT_EQ("0", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b11010011110000101011000110100000", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSyncHighResetInitSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_sync_high_reset_init_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_sync_high_reset_init_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_sync_high_reset_init_supported(
  input  logic       clk_i,
  input  logic       rst_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i) begin
    if (rst_i) begin
      mem_q <= '{8'h10, 8'h21, 8'h32, 8'h43};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_sync_high_reset_init_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("0", rstAsyncParam->getValue());
  EXPECT_EQ("0", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b01000011001100100010000100010000", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSyncLowBitwiseResetInitSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_sync_low_bitwise_reset_init_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_sync_low_bitwise_reset_init_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_sync_low_bitwise_reset_init_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i) begin
    if (~rst_ni) begin
      mem_q <= '{8'h55, 8'h66, 8'h77, 8'h88};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_sync_low_bitwise_reset_init_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("0", rstAsyncParam->getValue());
  EXPECT_EQ("1", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b10001000011101110110011001010101", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceAsyncResetConditionMismatchFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_async_reset_condition_mismatch_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_async_reset_condition_mismatch_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_async_reset_condition_mismatch_fallback(
  input  logic       clk_i,
  input  logic       rst_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or posedge rst_i) begin
    if (~rst_i) begin
      mem_q <= '{8'h00, 8'h11, 8'h22, 8'h33};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported sequential array assignment after async reset mismatch";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported RHS in sequential assignment"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSyncLogicalNotResetFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_sync_logical_not_reset_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_sync_logical_not_reset_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_sync_logical_not_reset_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i) begin
    if (!rst_ni) begin
      mem_q <= '{8'h00, 8'h11, 8'h22, 8'h33};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported sequential array assignment after sync logical-not reset";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported RHS in sequential assignment"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceResetInitUnknownBitsFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_unknown_bits_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_reset_init_unknown_bits_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_unknown_bits_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      mem_q <= '{8'hxx, 8'h11, 8'h22, 8'h33};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported sequential array assignment after unknown-bit reset init";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported RHS in sequential assignment"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceResetInitNonConstantFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_non_constant_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_reset_init_non_constant_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_non_constant_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= data_i;
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported sequential array assignment after non-constant reset init";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceResetInitNarrowEntryZeroExtendSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_narrow_entry_zero_extend_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_narrow_entry_zero_extend_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_narrow_entry_zero_extend_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (i == 0) begin
          mem_q[i] <= 4'h1;
        end else if (i == 1) begin
          mem_q[i] <= 4'h2;
        end else if (i == 2) begin
          mem_q[i] <= 4'h3;
        end else begin
          mem_q[i] <= 4'h4;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_narrow_entry_zero_extend_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b00000100000000110000001000000001", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceResetInitOversizedEntryFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_oversized_entry_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_reset_init_oversized_entry_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_oversized_entry_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 16'h1234;
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported sequential array assignment after oversized reset init";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceResetInitRealConstantsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_real_constants_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_reset_init_real_constants_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_real_constants_supported(
  input  logic        clk_i,
  input  logic        rst_i,
  input  logic [1:0]  addr_i,
  input  logic [63:0] data_i,
  output logic [63:0] data_o
);
  logic [63:0] mem_q [0:3];
  logic [63:0] mem_d [0:3];

  localparam real ZERO_R  = 0.0;
  localparam real ONE_R   = 1.0;
  localparam real TWO_R   = 2.0;
  localparam real THREE_R = 3.0;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or posedge rst_i) begin
    if (rst_i) begin
      for (int i = 0; i < 4; i++) begin
        if (i == 0) begin
          mem_q[i] <= ZERO_R;
        end else if (i == 1) begin
          mem_q[i] <= ONE_R;
        end else if (i == 2) begin
          mem_q[i] <= TWO_R;
        end else begin
          mem_q[i] <= THREE_R;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_reset_init_real_constants_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);

  const std::array<uint64_t, 4> values{0, 1, 2, 3};
  std::string expectedInit = "256'b";
  for (auto it = values.rbegin(); it != values.rend(); ++it) {
    for (int bit = 63; bit >= 0; --bit) {
      expectedInit += ((*it >> bit) & 1ULL) ? '1' : '0';
    }
  }
  EXPECT_EQ(expectedInit, initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitPackedArrayElementSelectSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_packed_array_element_select_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_packed_array_element_select_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_packed_array_element_select_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  localparam logic [0:3][7:0] INIT_PACKED = '{8'h11, 8'h22, 8'h33, 8'h44};

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (i == 0) begin
          mem_q[i] <= INIT_PACKED[0];
        end else if (i == 1) begin
          mem_q[i] <= INIT_PACKED[1];
        end else if (i == 2) begin
          mem_q[i] <= INIT_PACKED[2];
        end else begin
          mem_q[i] <= INIT_PACKED[3];
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_packed_array_element_select_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
  ASSERT_NE(nullptr, memoryInst->getInstParameter(NLName("INIT")));
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitStructMemberAccessSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_struct_member_access_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_struct_member_access_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_struct_member_access_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  typedef struct packed {
    logic [7:0] payload;
    logic       valid;
  } init_t;

  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  localparam init_t INIT0 = '{payload: 8'h12, valid: 1'b0};
  localparam init_t INIT1 = '{payload: 8'h23, valid: 1'b1};
  localparam init_t INIT2 = '{payload: 8'h34, valid: 1'b0};
  localparam init_t INIT3 = '{payload: 8'h45, valid: 1'b1};

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (i == 0) begin
          mem_q[i] <= INIT0.payload;
        end else if (i == 1) begin
          mem_q[i] <= INIT1.payload;
        end else if (i == 2) begin
          mem_q[i] <= INIT2.payload;
        end else begin
          mem_q[i] <= INIT3.payload;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_struct_member_access_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
  ASSERT_NE(nullptr, memoryInst->getInstParameter(NLName("INIT")));
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitRangeSelectSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_range_select_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_reset_init_range_select_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_range_select_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  localparam logic [31:0] INIT_WORD = 32'h44332211;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (i == 0) begin
          mem_q[i] <= INIT_WORD[7:0];
        end else if (i == 1) begin
          mem_q[i] <= INIT_WORD[15:8];
        end else if (i == 2) begin
          mem_q[i] <= INIT_WORD[23:16];
        end else begin
          mem_q[i] <= INIT_WORD[31:24];
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_reset_init_range_select_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
  ASSERT_NE(nullptr, memoryInst->getInstParameter(NLName("INIT")));
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitUnaryConstantConditionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_unary_constant_condition_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_unary_constant_condition_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_unary_constant_condition_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (!(i < 2)) begin
          mem_q[i] <= 8'hAA;
        end else begin
          mem_q[i] <= 8'h11;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_unary_constant_condition_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b10101010101010100001000100010001", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitLiteralConstantConditionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_literal_constant_condition_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_literal_constant_condition_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_literal_constant_condition_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (1'b1) begin
          mem_q[i] <= 8'hA5;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_literal_constant_condition_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b10100101101001011010010110100101", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitBitwiseNotConstantConditionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_bitwise_not_constant_condition_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_bitwise_not_constant_condition_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_bitwise_not_constant_condition_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (~(i < 2)) begin
          mem_q[i] <= 8'hAA;
        end else begin
          mem_q[i] <= 8'h11;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_bitwise_not_constant_condition_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b10101010101010100001000100010001", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitUnaryNonConstantConditionFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_unary_non_constant_condition_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_unary_non_constant_condition_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_unary_non_constant_condition_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (~addr_i[0]) begin
          mem_q[i] <= 8'hAA;
        end else begin
          mem_q[i] <= 8'h11;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitComparisonConstantConditionsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_comparison_constant_conditions_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_comparison_constant_conditions_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_comparison_constant_conditions_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (i > 3) begin
          mem_q[i] <= 8'hF0;
        end else if (i >= 2) begin
          mem_q[i] <= 8'hA0;
        end else if (i == 1) begin
          mem_q[i] <= 8'hB0;
        end else if (i <= 0) begin
          mem_q[i] <= 8'hC0;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_comparison_constant_conditions_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b10100000101000001011000011000000", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitInequalityConstantConditionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_inequality_constant_condition_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_inequality_constant_condition_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_inequality_constant_condition_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (i != 1) begin
          mem_q[i] <= 8'hD0;
        end else begin
          mem_q[i] <= 8'hE1;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_inequality_constant_condition_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b11010000110100001110000111010000", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitLogicalAndConditionFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_logical_and_condition_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_logical_and_condition_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_logical_and_condition_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if ((i < 2) && 1'b1) begin
          mem_q[i] <= 8'hAA;
        end else begin
          mem_q[i] <= 8'h11;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceConditionalSideLogicSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_conditional_side_logic_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_conditional_side_logic_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_conditional_side_logic_supported(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] alt_addr_i,
  input  logic [7:0] data_i,
  input  logic [7:0] alt_data_i,
  output logic [7:0] data_o,
  output logic       side_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    side_o = sel_i;
    mem_d = mem_q;
    if (sel_i) begin
      mem_d[addr_i] = data_i;
    end else begin
      mem_d[alt_addr_i] = alt_data_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_conditional_side_logic_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("side_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("2", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSameGuardPartialWritesSingleWEDriver) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_same_guard_partial_writes_single_we_driver";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_same_guard_partial_writes_single_we_driver.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_same_guard_partial_writes_single_we_driver(
  input  logic       clk_i,
  input  logic       valid_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] data_i,
  output logic [1:0] data_o
);
  logic [1:0] mem_q [0:3];
  logic [1:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (valid_i) begin
      mem_d[addr_i][0] = data_i[0];
      mem_d[addr_i][1] = data_i[1];
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_same_guard_partial_writes_single_we_driver"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("2", wrPortsParam->getValue());

  size_t checkedWriteEnableNets = 0;
  for (auto net : top->getNets()) {
    auto* bitNet = dynamic_cast<SNLBitNet*>(net);
    if (!bitNet || bitNet->isUnnamed()) {
      continue;
    }
    const auto name = bitNet->getName().getString();
    if (name.find("mem_we_") == std::string::npos) {
      continue;
    }
    ++checkedWriteEnableNets;
    EXPECT_EQ(1u, countAssignDrivers(top, bitNet)) << name;
  }
  EXPECT_EQ(2u, checkedWriteEnableNets);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceLoopConditionalElseSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_loop_conditional_else_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_loop_conditional_else_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_loop_conditional_else_supported(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    for (int i = 0; i < 4; i++) begin
      if (sel_i) begin
        mem_d[i] = data_i;
      end else begin
        mem_d[i] = 8'h00;
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_loop_conditional_else_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceLoopBreakSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_loop_break_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_loop_break_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_loop_break_supported(
  input  logic       clk_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    for (int i = 0; i < 4; i++) begin
      mem_d[i] = data_i;
      break;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_loop_break_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceCasezFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_casez_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_casez_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_casez_fallback(
  input  logic       clk_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    casez (mode_i)
      2'b1?: mem_d[0] = data_i;
      default: mem_d[addr_i] = data_i;
    endcase
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceIfFalseBranchFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_iffalse_branch_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_iffalse_branch_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_iffalse_branch_failure_fallback(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      mem_d[addr_i] = data_i;
    end else begin
      mem_d[addr_i] += data_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceCaseItemFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_case_item_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_case_item_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_case_item_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    case (mode_i)
      2'd0: mem_d[addr_i] = data_i;
      2'd1: mem_d[addr_i] += data_i;
      default: mem_d[0] = data_i;
    endcase
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceCaseDefaultFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_case_default_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_case_default_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_case_default_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    case (mode_i)
      2'd0: mem_d[addr_i] = data_i;
      2'd1: mem_d[0] = data_i;
      default: mem_d[addr_i] += data_i;
    endcase
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceConditionGuardFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_condition_guard_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_condition_guard_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_condition_guard_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if ($urandom_range(1, 0)) begin
      mem_d[addr_i] = data_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceNoIndexedWritesAfterLateConstantFalseGuardFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_no_indexed_writes_after_late_constant_false_guard";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_no_indexed_writes_after_late_constant_false_guard.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_no_indexed_writes_after_late_constant_false_guard(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (addr_i[0] && 1'b0) begin
      mem_d[addr_i] = data_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceCaseItemGuardFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_case_item_guard_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_case_item_guard_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_case_item_guard_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    case (mode_i)
      $urandom_range(1, 0): mem_d[addr_i] = data_i;
      default: mem_d[0] = data_i;
    endcase
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceCompoundShadowActionFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_compound_shadow_action_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_compound_shadow_action_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_compound_shadow_action_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] += data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceNestedBaseCopyFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_nested_base_copy_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_nested_base_copy_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_nested_base_copy_fallback(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      mem_d = mem_q;
    end
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceCaseAndForSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_case_and_for_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_case_and_for_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_case_and_for_supported(
  input  logic       clk_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  input  logic [1:0] token_i,
  output logic [7:0] data_o,
  output logic       flag_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    flag_o = 1'b0;
    mem_d = mem_q;
    case (mode_i)
      2'b00: begin
        for (int i = 0; i < 2; i++) begin
          mem_d[i] = {7'b0, token_i[i]};
        end
      end
      2'b01: begin
        mem_d[addr_i] = data_i;
      end
      default: begin
        flag_o = 1'b1;
      end
    endcase
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_case_and_for_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("flag_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("3", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceConstantConditionalsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_constant_conditionals_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_constant_conditionals_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_constant_conditionals_supported(
  input  logic       clk_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (1'b0) begin
      mem_d[0] = 8'hAA;
    end
    if (1'b1) begin
      mem_d[1] = data_i;
    end else begin
      mem_d[1] = 8'h55;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[1];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_constant_conditionals_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSharedSequentialResetLoopSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_reset_loop_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_shared_sequential_reset_loop_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_reset_loop_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic       flag_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic       flag_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      flag_q <= 1'b0;
      for (int i = 0; i < 4; i++) begin
        if (i == 0) begin
          mem_q[i] <= 8'h11;
        end else if (i == 1) begin
          mem_q[i] <= 8'h22;
        end else begin
          mem_q[i] <= 8'h00;
        end
      end
    end else begin
      flag_q <= en_i;
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
  assign flag_o = flag_q;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_reset_loop_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("flag_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("1", rstAsyncParam->getValue());
  EXPECT_EQ("1", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b00000000000000000010001000010001", initParam->getValue());

  auto dumpedVerilog = dumpTopAndGetVerilogPath(
    top, "qd_memory_inference_shared_sequential_reset_loop_supported");
  std::string dumpedText = readTextFile(dumpedVerilog);
  EXPECT_NE(std::string::npos, dumpedText.find("naja_mem #("));
  EXPECT_EQ(std::string::npos, dumpedText.find("module naja_mem #("));
  EXPECT_NE(
    std::string::npos,
    dumpedText.find(".INIT(32'b00000000000000000010001000010001)"));
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialIgnoresUnrelatedAlwaysFFChainSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_ignores_unrelated_alwaysff_chain";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_ignores_unrelated_alwaysff_chain.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_ignores_unrelated_alwaysff_chain(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic       scratch_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic       scratch_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni)
      scratch_q <= 1'b0;
    else
      scratch_q <= en_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
  assign scratch_o = scratch_q;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_ignores_unrelated_alwaysff_chain"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("scratch_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseGeneratedDirectSequentialMemoryWriteInferenceSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "generated_direct_sequential_memory_write_inference_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "generated_direct_sequential_memory_write_inference_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module generated_direct_sequential_memory_write_inference_supported #(
  parameter int width_p = 8,
  parameter int els_p = 4,
  parameter int addr_width_lp = 2
) (
  input  logic                    clk_i,
  input  logic                    w_v_i,
  input  logic [addr_width_lp-1:0] w_addr_i,
  input  logic [width_p-1:0]      w_data_i,
  input  logic [addr_width_lp-1:0] r_addr_i,
  output logic [width_p-1:0]      r_data_o
);
  if (width_p == 0 || els_p == 0) begin : z
    assign r_data_o = '0;
  end else begin : nz
    logic [width_p-1:0] mem [els_p-1:0];
    wire [addr_width_lp-1:0] r_addr_li = (els_p > 0) ? r_addr_i : '0;
    wire [addr_width_lp-1:0] w_addr_li = (els_p > 0) ? w_addr_i : '0;

    assign r_data_o = mem[r_addr_li];

    always_ff @(posedge clk_i) begin
      if (w_v_i) begin
        mem[w_addr_li] <= w_data_i;
      end
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("generated_direct_sequential_memory_write_inference_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  auto* depthParam = memoryInst->getInstParameter(NLName("DEPTH"));
  ASSERT_NE(nullptr, widthParam);
  ASSERT_NE(nullptr, depthParam);
  EXPECT_EQ("8", widthParam->getValue());
  EXPECT_EQ("4", depthParam->getValue());
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseDirectSequentialMemoryMaskedBitWriteInferenceSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "direct_sequential_memory_masked_bit_write_inference_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "direct_sequential_memory_masked_bit_write_inference_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module direct_sequential_memory_masked_bit_write_inference_supported #(
  parameter int width_p = 8,
  parameter int els_p = 4,
  parameter int addr_width_lp = 2
) (
  input  logic                     clk_i,
  input  logic                     w_v_i,
  input  logic [addr_width_lp-1:0] w_addr_i,
  input  logic [width_p-1:0]       w_data_i,
  input  logic [width_p-1:0]       w_mask_i,
  input  logic [addr_width_lp-1:0] r_addr_i,
  output logic [width_p-1:0]       r_data_o
);
  logic [width_p-1:0] mem [els_p-1:0];

  assign r_data_o = mem[r_addr_i];

  always_ff @(posedge clk_i)
    if (w_v_i)
      for (integer i = 0; i < width_p; i=i+1)
        if (w_mask_i[i])
          mem[w_addr_i][i] <= w_data_i[i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("direct_sequential_memory_masked_bit_write_inference_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseDirectSequentialMemoryAsyncLowResetInitSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "direct_sequential_memory_async_low_reset_init_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "direct_sequential_memory_async_low_reset_init_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module direct_sequential_memory_async_low_reset_init_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       w_v_i,
  input  logic [1:0] w_addr_i,
  input  logic [7:0] w_data_i,
  output logic [7:0] r_data_o
);
  logic [7:0] mem [0:3];

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      mem <= '{default: '0};
    end else begin
      for (int i = 0; i < 4; i++) begin
        if (w_v_i && (w_addr_i == i[1:0])) begin
          mem[i] <= w_data_i;
        end
      end
    end
  end

  assign r_data_o = mem[w_addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("direct_sequential_memory_async_low_reset_init_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("1", rstAsyncParam->getValue());
  EXPECT_EQ("1", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b00000000000000000000000000000000", initParam->getValue());
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialWholeArrayCopyIgnoresUnaryExpressionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "qd_memory_inference_shared_sequential_whole_array_copy_ignores_unary_expression";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_whole_array_copy_ignores_unary_expression.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_whole_array_copy_ignores_unary_expression(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic [1:0] scratch_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    scratch_o = 2'b00;
    scratch_o++;
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni)
      mem_q <= '{8'h11, 8'h22, 8'h33, 8'h44};
    else
      mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_whole_array_copy_ignores_unary_expression"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("scratch_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialMissingDefaultChainFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_missing_default_chain";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_missing_default_chain.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_missing_default_chain(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni)
      mem_q <= '{8'h11, 8'h22, 8'h33, 8'h44};
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor, svPath, {"Unsupported RHS in sequential assignment"});
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialInvalidDefaultCommitTargetFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_invalid_default_commit_target";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_invalid_default_commit_target.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_invalid_default_commit_target(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni)
      mem_q <= '{8'h11, 8'h22, 8'h33, 8'h44};
    else
      mem_q <= mem_q;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor, svPath, {"Unsupported RHS in sequential assignment"});
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialSimpleResetChainSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_simple_reset_chain";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_simple_reset_chain.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_simple_reset_chain(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni)
      mem_q <= '{8'h11, 8'h22, 8'h33, 8'h44};
    else
      mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_simple_reset_chain"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("1", rstAsyncParam->getValue());
  EXPECT_EQ("1", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b01000100001100110010001000010001", initParam->getValue());
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialResetConstantFalseNoElseSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_reset_constant_false_no_else";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_reset_constant_false_no_else.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_reset_constant_false_no_else(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic       flag_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic       flag_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      flag_q <= 1'b0;
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
        if (1'b0) begin
          mem_q[i] <= 8'hAA;
        end
      end
    end else begin
      flag_q <= en_i;
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
  assign flag_o = flag_q;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_reset_constant_false_no_else"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("flag_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b00000000000000000000000000000000", initParam->getValue());
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialResetIgnoresNonAssignmentSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_reset_ignores_non_assignment";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_reset_ignores_non_assignment.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_reset_ignores_non_assignment(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic       flag_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      flag_q <= 1'b0;
      case (addr_i)
        default: ;
      endcase
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      flag_q <= en_i;
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_reset_ignores_non_assignment"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialResetCaseAssignsTrackedLhsFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_reset_case_assigns_tracked_lhs";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_reset_case_assigns_tracked_lhs.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_reset_case_assigns_tracked_lhs(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic       flag_o,
  output logic       other_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic       flag_q;
  logic       other_q;
  logic       other_n;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
    other_n = other_q;
    if (en_i) begin
      other_n = ~other_q;
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      flag_q <= 1'b0;
      other_q <= 1'b0;
      case (addr_i)
        default: flag_q <= 1'b1;
      endcase
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      flag_q <= en_i;
      other_q <= other_n;
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
  assign flag_o = flag_q;
  assign other_o = other_q;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_reset_case_assigns_tracked_lhs"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialDynamicResetIndexFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_dynamic_reset_index_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_dynamic_reset_index_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_dynamic_reset_index_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      mem_q[addr_i] <= 8'h00;
    end else begin
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected dynamic reset index fallback";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialResetMissingTargetAssignmentFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_reset_missing_target_assignment";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_reset_missing_target_assignment.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_reset_missing_target_assignment(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic       flag_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic       flag_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      flag_q <= 1'b0;
    end else begin
      flag_q <= en_i;
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
  assign flag_o = flag_q;
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected missing reset assignment fallback";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialResetPartialInitializationFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_reset_partial_initialization";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_reset_partial_initialization.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_reset_partial_initialization(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      mem_q[0] <= 8'h00;
    end else begin
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected partial reset initialization fallback";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialResetCompoundAssignmentFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_reset_compound_assignment";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_reset_compound_assignment.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_reset_compound_assignment(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      mem_q[0] += 8'h01;
    end else begin
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected compound reset assignment fallback";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialSelfCommitTargetFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_self_commit_target_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_self_commit_target_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_self_commit_target_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      mem_q <= mem_q;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected self commit target fallback";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialMismatchedCommitSignatureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_mismatched_commit_signature";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_mismatched_commit_signature.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_mismatched_commit_signature(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic [9:0] mem_next_bad [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
    for (int i = 0; i < 4; i++) begin
      mem_next_bad[i] = {2'b00, mem_d[i]};
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      mem_q <= mem_next_bad;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected mismatched commit signature fallback";
  } catch (const SNLSVConstructorException& e) {
    EXPECT_FALSE(std::string(e.what()).empty());
  }
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialIgnoresTopLevelConditionalSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_ignores_top_level_conditional";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_ignores_top_level_conditional.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_ignores_top_level_conditional(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic       flag_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic       flag_q;
  logic       scratch_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      flag_q <= 1'b0;
      scratch_q <= 1'b0;
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      flag_q <= en_i;
      if (en_i) begin
        scratch_q <= addr_i[0];
      end
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
  assign flag_o = flag_q ^ scratch_q;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_ignores_top_level_conditional"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("flag_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialIgnoresNonAssignmentExpressionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_ignores_non_assignment_expression";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_ignores_non_assignment_expression.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_ignores_non_assignment_expression(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic [1:0] scratch_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic [1:0] scratch_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      scratch_q <= 2'b00;
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      ++scratch_q;
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
  assign scratch_o = scratch_q;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_ignores_non_assignment_expression"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("scratch_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialMultipleTargetAssignmentsFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_multiple_target_assignments_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_multiple_target_assignments_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_multiple_target_assignments_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      mem_q <= mem_next;
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected shared sequential duplicate target assignment fallback";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextMemoryInferenceSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_memory_inference_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_memory_inference_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_memory_inference_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_memory_inference_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIndexedCommitMemoryInferenceSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_memory_inference_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_memory_inference_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_memory_inference_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (i < 2) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_indexed_commit_memory_inference_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIndexedCommitElseGuardSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_else_guard_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_else_guard_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_else_guard_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (mem_d[i][0]) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_indexed_commit_else_guard_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceArithmeticIndexKeepsInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_arithmetic_index_keeps_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
      outPath / "qd_memory_inference_arithmetic_index_keeps_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_arithmetic_index_keeps_internal_inputs_driven(
  input  logic        clk_i,
  input  logic [11:0] addr_i,
  input  logic [31:0] data_i,
  output logic [63:0] data_o
);
  logic [63:0] mem_q [0:63];
  logic [63:0] mem_d [0:63];
  logic [63:0] mem_next [0:63];
  logic [5:0]  slot_idx;

  always_comb begin
    slot_idx = addr_i - 12'hB03 + 12'd1;
    mem_d = mem_q;
    mem_d[slot_idx][31:0] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 64; i++) begin
      if (i < 32) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[slot_idx];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
      NLName("qd_memory_inference_arithmetic_index_keeps_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  size_t memoryCount = 0;
  for (auto* inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ++memoryCount;
    }
  }
  EXPECT_EQ(1u, memoryCount);

  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parsePerfCounterStyleMemoryInferenceKeepsInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "perf_counter_style_memory_inference_keeps_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
      outPath / "perf_counter_style_memory_inference_keeps_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module perf_counter_style_memory_inference_keeps_internal_inputs_driven(
  input  logic        clk_i,
  input  logic        rst_ni,
  input  logic [11:0] addr_i,
  input  logic        we_i,
  input  logic [31:0] data_i,
  output logic [63:0] data_o
);
  localparam int unsigned CounterNum = 8;
  localparam logic [11:0] CounterBase = 12'hB03;

  logic [63:0] generic_counter_d[CounterNum:1];
  logic [63:0] generic_counter_q[CounterNum:1];

  always_comb begin
    generic_counter_d = generic_counter_q;
    data_o = '0;

    for (int unsigned i = 1; i <= CounterNum; i++) begin
      generic_counter_d[i] = generic_counter_q[i] + 1'b1;
    end

    if ((addr_i >= CounterBase) && (addr_i < (CounterBase + CounterNum))) begin
      data_o = generic_counter_q[addr_i - CounterBase + 1];
    end

    if (we_i) begin
      if ((addr_i >= CounterBase) && (addr_i < (CounterBase + CounterNum))) begin
        generic_counter_d[addr_i - CounterBase + 1][31:0] = data_i;
      end
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      generic_counter_q <= '{default: 0};
    end else begin
      generic_counter_q <= generic_counter_d;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
      NLName("perf_counter_style_memory_inference_keeps_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  size_t memoryCount = 0;
  for (auto* inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ++memoryCount;
    }
  }
  EXPECT_EQ(1u, memoryCount);

  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parsePerfCounterStyleMemoryInferenceKeepsMemoryReadOutputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "perf_counter_style_memory_inference_keeps_memory_read_outputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
      outPath / "perf_counter_style_memory_inference_keeps_memory_read_outputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module perf_counter_style_memory_inference_keeps_memory_read_outputs_driven(
  input  logic        clk_i,
  input  logic        rst_ni,
  input  logic [11:0] addr_i,
  input  logic        we_i,
  input  logic [31:0] data_i,
  output logic [63:0] data_o
);
  localparam int unsigned CounterNum = 8;
  localparam logic [11:0] CounterBase = 12'hB03;

  logic [63:0] generic_counter_d[CounterNum:1];
  logic [63:0] generic_counter_q[CounterNum:1];

  always_comb begin
    generic_counter_d = generic_counter_q;
    data_o = '0;

    for (int unsigned i = 1; i <= CounterNum; i++) begin
      generic_counter_d[i] = generic_counter_q[i] + 1'b1;
    end

    if ((addr_i >= CounterBase) && (addr_i < (CounterBase + CounterNum))) begin
      data_o = generic_counter_q[addr_i - CounterBase + 1];
    end

    if (we_i) begin
      if ((addr_i >= CounterBase) && (addr_i < (CounterBase + CounterNum))) begin
        generic_counter_d[addr_i - CounterBase + 1][31:0] = data_i;
      end
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      generic_counter_q <= '{default: 0};
    end else begin
      generic_counter_q <= generic_counter_d;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
      NLName("perf_counter_style_memory_inference_keeps_memory_read_outputs_driven"));
  ASSERT_NE(top, nullptr);

  const auto undrivenReadOutputs = collectUndrivenMemoryReadOutputs(top);
  EXPECT_TRUE(undrivenReadOutputs.empty()) << formatStringVector(undrivenReadOutputs);
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseCva6PerfCounterStyleMemoryInferenceKeepsInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "cva6_perf_counter_style_memory_inference_keeps_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
      outPath / "cva6_perf_counter_style_memory_inference_keeps_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module cva6_perf_counter_style_memory_inference_keeps_internal_inputs_driven(
  input  logic        clk_i,
  input  logic        rst_ni,
  input  logic        debug_mode_i,
  input  logic [11:0] addr_i,
  input  logic        we_i,
  input  logic [31:0] data_i,
  input  logic [31:0] mcountinhibit_i,
  output logic [31:0] data_o
);
  localparam int unsigned CounterNum = 8;
  localparam logic [11:0] CounterBaseLo = 12'hB03;
  localparam logic [11:0] CounterBaseHi = 12'hB83;
  localparam logic [11:0] EventBase = 12'h323;

  logic [63:0] generic_counter_d[CounterNum:1];
  logic [63:0] generic_counter_q[CounterNum:1];
  logic [4:0]  mhpmevent_d[CounterNum:1];
  logic [4:0]  mhpmevent_q[CounterNum:1];
  logic [CounterNum:1] events;

  always_comb begin
    events = '0;
    for (int unsigned i = 1; i <= CounterNum; i++) begin
      events[i] = i[0];
    end
  end

  always_comb begin
    generic_counter_d = generic_counter_q;
    mhpmevent_d = mhpmevent_q;
    data_o = '0;

    for (int unsigned i = 1; i <= CounterNum; i++) begin
      if ((!debug_mode_i) && (!we_i)) begin
        if (events[i] && !mcountinhibit_i[i + 2]) begin
          generic_counter_d[i] = generic_counter_q[i] + 1'b1;
        end
      end
    end

    if ((addr_i >= CounterBaseLo) && (addr_i < (CounterBaseLo + CounterNum))) begin
      data_o = generic_counter_q[addr_i - CounterBaseLo + 1][31:0];
    end else if ((addr_i >= CounterBaseHi) && (addr_i < (CounterBaseHi + CounterNum))) begin
      data_o = generic_counter_q[addr_i - CounterBaseHi + 1][63:32];
    end else if ((addr_i >= EventBase) && (addr_i < (EventBase + CounterNum))) begin
      data_o = {27'b0, mhpmevent_q[addr_i - EventBase + 1]};
    end

    if (we_i) begin
      if ((addr_i >= CounterBaseLo) && (addr_i < (CounterBaseLo + CounterNum))) begin
        generic_counter_d[addr_i - CounterBaseLo + 1][31:0] = data_i;
      end else if ((addr_i >= CounterBaseHi) && (addr_i < (CounterBaseHi + CounterNum))) begin
        generic_counter_d[addr_i - CounterBaseHi + 1][63:32] = data_i;
      end else if ((addr_i >= EventBase) && (addr_i < (EventBase + CounterNum))) begin
        mhpmevent_d[addr_i - EventBase + 1] = data_i[4:0];
      end
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      generic_counter_q <= '{default: 0};
      mhpmevent_q <= '{default: 0};
    end else begin
      generic_counter_q <= generic_counter_d;
      mhpmevent_q <= mhpmevent_d;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
      NLName("cva6_perf_counter_style_memory_inference_keeps_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  // Guard the CVA6 perf-counter regression: lower/upper CSR windows plus event
  // writes must not lower inferred-memory WE/WDATA inputs onto undriven internal
  // nets that later show up as SEC no-driver boundary failures.
  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseCva6PerfCounterFullDecodeStyleMemoryInferenceKeepsInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "cva6_perf_counter_full_decode_style_memory_inference_keeps_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "cva6_perf_counter_full_decode_style_memory_inference_keeps_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module cva6_perf_counter_full_decode_style_memory_inference_keeps_internal_inputs_driven(
  input  logic        clk_i,
  input  logic        rst_ni,
  input  logic        debug_mode_i,
  input  logic [11:0] addr_i,
  input  logic        we_i,
  input  logic [63:0] data_i,
  input  logic [31:0] mcountinhibit_i,
  output logic [63:0] data_o
);
  localparam int unsigned XLEN = 64;
  localparam int unsigned MHPMCounterNum = 6;
  typedef logic [11:0] csr_addr_t;

  localparam csr_addr_t CSR_MHPM_COUNTER_3  = 12'hB03;
  localparam csr_addr_t CSR_MHPM_COUNTER_3H = 12'hB83;
  localparam csr_addr_t CSR_MHPM_EVENT_3    = 12'h323;
  localparam csr_addr_t CSR_HPM_COUNTER_3   = 12'hC03;
  localparam csr_addr_t CSR_HPM_COUNTER_3H  = 12'hC83;

  logic [63:0] generic_counter_d[MHPMCounterNum:1];
  logic [63:0] generic_counter_q[MHPMCounterNum:1];
  logic [4:0]  mhpmevent_d[MHPMCounterNum:1];
  logic [4:0]  mhpmevent_q[MHPMCounterNum:1];
  logic        events[MHPMCounterNum:1];
  logic        read_access_exception;
  logic        update_access_exception;

  always_comb begin : mux_events
    events[MHPMCounterNum:1] = '{default: 1'b0};
    for (int unsigned i = 1; i <= MHPMCounterNum; i++) begin
      case (mhpmevent_q[i])
        5'b00000: events[i] = 1'b0;
        5'b00001: events[i] = i[0];
        5'b00010: events[i] = addr_i[0];
        default:  events[i] = we_i;
      endcase
    end
  end

  always_comb begin : generic_counter
    generic_counter_d = generic_counter_q;
    data_o = '0;
    mhpmevent_d = mhpmevent_q;
    read_access_exception = 1'b0;
    update_access_exception = 1'b0;

    for (int unsigned i = 1; i <= MHPMCounterNum; i++) begin
      if ((!debug_mode_i) && (!we_i)) begin
        if (events[i] && (!mcountinhibit_i[i+2])) begin
          generic_counter_d[i] = generic_counter_q[i] + 1'b1;
        end
      end
    end

    if ((addr_i >= CSR_MHPM_COUNTER_3) &&
        (addr_i < (CSR_MHPM_COUNTER_3 + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o[31:0] = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3+1][31:0];
      end else begin
        data_o = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3+1];
      end
    end else if ((addr_i >= CSR_MHPM_COUNTER_3H) &&
                 (addr_i < (CSR_MHPM_COUNTER_3H + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o[31:0] = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3H+1][63:32];
      end else begin
        read_access_exception = 1'b1;
      end
    end else if ((addr_i >= CSR_MHPM_EVENT_3) &&
                 (addr_i < (CSR_MHPM_EVENT_3 + MHPMCounterNum))) begin
      data_o[4:0] = mhpmevent_q[addr_i-CSR_MHPM_EVENT_3+1];
    end else if ((addr_i >= CSR_HPM_COUNTER_3) &&
                 (addr_i < (CSR_HPM_COUNTER_3 + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o[31:0] = generic_counter_q[addr_i-CSR_HPM_COUNTER_3+1][31:0];
      end else begin
        data_o = generic_counter_q[addr_i-CSR_HPM_COUNTER_3+1];
      end
    end else if ((addr_i > CSR_HPM_COUNTER_3H) &&
                 (addr_i < (CSR_HPM_COUNTER_3H + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o[31:0] = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3H+1][63:32];
      end else begin
        read_access_exception = 1'b1;
      end
    end

    if (we_i) begin
      if ((addr_i >= CSR_MHPM_COUNTER_3) &&
          (addr_i < (CSR_MHPM_COUNTER_3 + MHPMCounterNum))) begin
        if (XLEN == 32) begin
          generic_counter_d[addr_i-CSR_MHPM_COUNTER_3+1][31:0] = data_i[31:0];
        end else begin
          generic_counter_d[addr_i-CSR_MHPM_COUNTER_3+1] = data_i;
        end
      end else if ((addr_i >= CSR_MHPM_COUNTER_3H) &&
                   (addr_i < (CSR_MHPM_COUNTER_3H + MHPMCounterNum))) begin
        if (XLEN == 32) begin
          generic_counter_d[addr_i-CSR_MHPM_COUNTER_3H+1][63:32] = data_i[31:0];
        end else begin
          update_access_exception = 1'b1;
        end
      end else if ((addr_i >= CSR_MHPM_EVENT_3) &&
                   (addr_i < (CSR_MHPM_EVENT_3 + MHPMCounterNum))) begin
        mhpmevent_d[addr_i-CSR_MHPM_EVENT_3+1] = data_i[4:0];
      end
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      generic_counter_q <= '{default: 0};
      mhpmevent_q <= '{default: 0};
    end else begin
      generic_counter_q <= generic_counter_d;
      mhpmevent_q <= mhpmevent_d;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName(
      "cva6_perf_counter_full_decode_style_memory_inference_keeps_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  // Guard the fuller CVA6 performance-counter decode shape: mixed full-word,
  // lower-half, upper-half, and event-window accesses must not leave inferred
  // memory write enables or write-data mux inputs undriven.
  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseCva6PerfCounterEventEqualityStyleMemoryInferenceKeepsInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath /
    "cva6_perf_counter_event_equality_style_memory_inference_keeps_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "cva6_perf_counter_event_equality_style_memory_inference_keeps_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module cva6_perf_counter_event_equality_style_memory_inference_keeps_internal_inputs_driven(
  input  logic        clk_i,
  input  logic        rst_ni,
  input  logic        debug_mode_i,
  input  logic [11:0] addr_i,
  input  logic        we_i,
  input  logic [63:0] data_i,
  input  logic [31:0] mcountinhibit_i,
  input  logic [7:0]  event_select_i,
  output logic [63:0] data_o
);
  localparam int unsigned XLEN = 64;
  localparam int unsigned MHPMCounterNum = 6;
  typedef logic [11:0] csr_addr_t;

  localparam csr_addr_t CSR_MHPM_COUNTER_3  = 12'hB03;
  localparam csr_addr_t CSR_MHPM_COUNTER_3H = 12'hB83;
  localparam csr_addr_t CSR_MHPM_EVENT_3    = 12'h323;
  localparam csr_addr_t CSR_HPM_COUNTER_3   = 12'hC03;
  localparam csr_addr_t CSR_HPM_COUNTER_3H  = 12'hC83;

  logic [63:0] generic_counter_d[MHPMCounterNum:1];
  logic [63:0] generic_counter_q[MHPMCounterNum:1];
  logic [4:0]  mhpmevent_d[MHPMCounterNum:1];
  logic [4:0]  mhpmevent_q[MHPMCounterNum:1];
  logic        events[MHPMCounterNum:1];
  logic        read_access_exception;
  logic        update_access_exception;

  always_comb begin : mux_events
    events[MHPMCounterNum:1] = '{default: 1'b0};
    for (int unsigned i = 1; i <= MHPMCounterNum; i++) begin
      case (mhpmevent_q[i])
        5'b00000: events[i] = 1'b0;
        5'b00001: events[i] = event_select_i[i];
        5'b00010: events[i] = addr_i[i % 12];
        5'b00011: events[i] = we_i;
        5'b00100: events[i] = debug_mode_i;
        default:  events[i] = event_select_i[(i + 1) % 8];
      endcase
    end
  end

  always_comb begin : generic_counter
    generic_counter_d = generic_counter_q;
    data_o = '0;
    mhpmevent_d = mhpmevent_q;
    read_access_exception = 1'b0;
    update_access_exception = 1'b0;

    for (int unsigned i = 1; i <= MHPMCounterNum; i++) begin
      if ((!debug_mode_i) && (!we_i)) begin
        // Guard the CVA6 regression where the real perf-counter RTL uses an
        // explicit single-bit equality here. That comparison must not lower
        // inferred-memory write enables through an undriven xnor input.
        if (((events[i]) == 1'b1) && (!mcountinhibit_i[i+2])) begin
          generic_counter_d[i] = generic_counter_q[i] + 1'b1;
        end
      end
    end

    if ((addr_i >= CSR_MHPM_COUNTER_3) &&
        (addr_i < (CSR_MHPM_COUNTER_3 + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o[31:0] = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3+1][31:0];
      end else begin
        data_o = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3+1];
      end
    end else if ((addr_i >= CSR_MHPM_COUNTER_3H) &&
                 (addr_i < (CSR_MHPM_COUNTER_3H + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o[31:0] = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3H+1][63:32];
      end else begin
        read_access_exception = 1'b1;
      end
    end else if ((addr_i >= CSR_MHPM_EVENT_3) &&
                 (addr_i < (CSR_MHPM_EVENT_3 + MHPMCounterNum))) begin
      data_o[4:0] = mhpmevent_q[addr_i-CSR_MHPM_EVENT_3+1];
    end else if ((addr_i >= CSR_HPM_COUNTER_3) &&
                 (addr_i < (CSR_HPM_COUNTER_3 + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o[31:0] = generic_counter_q[addr_i-CSR_HPM_COUNTER_3+1][31:0];
      end else begin
        data_o = generic_counter_q[addr_i-CSR_HPM_COUNTER_3+1];
      end
    end else if ((addr_i > CSR_HPM_COUNTER_3H) &&
                 (addr_i < (CSR_HPM_COUNTER_3H + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o[31:0] = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3H+1][63:32];
      end else begin
        read_access_exception = 1'b1;
      end
    end

    if (we_i) begin
      if ((addr_i >= CSR_MHPM_COUNTER_3) &&
          (addr_i < (CSR_MHPM_COUNTER_3 + MHPMCounterNum))) begin
        if (XLEN == 32) begin
          generic_counter_d[addr_i-CSR_MHPM_COUNTER_3+1][31:0] = data_i[31:0];
        end else begin
          generic_counter_d[addr_i-CSR_MHPM_COUNTER_3+1] = data_i;
        end
      end else if ((addr_i >= CSR_MHPM_COUNTER_3H) &&
                   (addr_i < (CSR_MHPM_COUNTER_3H + MHPMCounterNum))) begin
        if (XLEN == 32) begin
          generic_counter_d[addr_i-CSR_MHPM_COUNTER_3H+1][63:32] = data_i[31:0];
        end else begin
          update_access_exception = 1'b1;
        end
      end else if ((addr_i >= CSR_MHPM_EVENT_3) &&
                   (addr_i < (CSR_MHPM_EVENT_3 + MHPMCounterNum))) begin
        mhpmevent_d[addr_i-CSR_MHPM_EVENT_3+1] = data_i[4:0];
      end
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      generic_counter_q <= '{default: 0};
      mhpmevent_q <= '{default: 0};
    end else begin
      generic_counter_q <= generic_counter_d;
      mhpmevent_q <= mhpmevent_d;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName(
      "cva6_perf_counter_event_equality_style_memory_inference_keeps_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseNearLiteralCva6PerfCountersMemoryInferenceKeepsInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath /
    "near_literal_cva6_perf_counters_memory_inference_keeps_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "near_literal_cva6_perf_counters_memory_inference_keeps_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module near_literal_cva6_perf_counters_memory_inference_keeps_internal_inputs_driven(
  input logic clk_i,
  input logic rst_ni,
  input logic debug_mode_i,
  input logic [11:0] addr_i,
  input logic we_i,
  input logic [63:0] data_i,
  output logic [63:0] data_o,
  input logic [1:0][3:0] commit_fu_i,
  input logic [1:0][3:0] commit_op_i,
  input logic [1:0][4:0] commit_rd_i,
  input logic [1:0] commit_ack_i,
  input logic l1_icache_miss_i,
  input logic l1_dcache_miss_i,
  input logic itlb_miss_i,
  input logic dtlb_miss_i,
  input logic sb_full_i,
  input logic if_empty_i,
  input logic ex_valid_i,
  input logic eret_i,
  input logic resolved_branch_valid_i,
  input logic resolved_branch_is_mispredict_i,
  input logic branch_exception_valid_i,
  input logic l1_icache_req_i,
  input logic [2:0] l1_dcache_req_i,
  input logic [2:0][7:0] miss_vld_bits_i,
  input logic i_tlb_flush_i,
  input logic stall_issue_i,
  input logic [31:0] mcountinhibit_i
);
  localparam int unsigned XLEN = 64;
  localparam int unsigned MHPMCounterNum = 6;
  localparam int unsigned NrCommitPorts = 2;
  localparam int unsigned NumPorts = 3;

  localparam logic [3:0] LOAD = 4'd1;
  localparam logic [3:0] STORE = 4'd2;
  localparam logic [3:0] CTRL_FLOW = 4'd3;
  localparam logic [3:0] ALU = 4'd4;
  localparam logic [3:0] MULT = 4'd5;
  localparam logic [3:0] FPU = 4'd6;
  localparam logic [3:0] FPU_VEC = 4'd7;
  localparam logic [3:0] ADD = 4'd8;
  localparam logic [3:0] JALR = 4'd9;

  typedef logic [11:0] csr_addr_t;
  localparam csr_addr_t CSR_MHPM_COUNTER_3 = 12'hB03;
  localparam csr_addr_t CSR_MHPM_COUNTER_3H = 12'hB83;
  localparam csr_addr_t CSR_MHPM_EVENT_3 = 12'h323;
  localparam csr_addr_t CSR_HPM_COUNTER_3 = 12'hC03;
  localparam csr_addr_t CSR_HPM_COUNTER_3H = 12'hC83;

  logic [63:0] generic_counter_d[MHPMCounterNum:1];
  logic [63:0] generic_counter_q[MHPMCounterNum:1];
  logic read_access_exception, update_access_exception;
  logic events[MHPMCounterNum:1];
  logic [4:0] mhpmevent_d[MHPMCounterNum:1];
  logic [4:0] mhpmevent_q[MHPMCounterNum:1];
  logic [NrCommitPorts-1:0] load_event;
  logic [NrCommitPorts-1:0] store_event;
  logic [NrCommitPorts-1:0] branch_event;
  logic [NrCommitPorts-1:0] call_event;
  logic [NrCommitPorts-1:0] return_event;
  logic [NrCommitPorts-1:0] int_event;
  logic [NrCommitPorts-1:0] fp_event;

  always_comb begin : Mux
    events[MHPMCounterNum:1] = '{default: 0};
    load_event = '{default: 0};
    store_event = '{default: 0};
    branch_event = '{default: 0};
    call_event = '{default: 0};
    return_event = '{default: 0};
    int_event = '{default: 0};
    fp_event = '{default: 0};

    for (int unsigned j = 0; j < NrCommitPorts; j++) begin
      load_event[j] = commit_ack_i[j] & (commit_fu_i[j] == LOAD);
      store_event[j] = commit_ack_i[j] & (commit_fu_i[j] == STORE);
      branch_event[j] = commit_ack_i[j] & (commit_fu_i[j] == CTRL_FLOW);
      call_event[j] = commit_ack_i[j] & (commit_fu_i[j] == CTRL_FLOW &&
                                         (commit_op_i[j] == ADD || commit_op_i[j] == JALR) &&
                                         (commit_rd_i[j] == 5'd1 || commit_rd_i[j] == 5'd5));
      return_event[j] = commit_ack_i[j] & (commit_op_i[j] == JALR && commit_rd_i[j] == 5'd0);
      int_event[j] = commit_ack_i[j] & (commit_fu_i[j] == ALU || commit_fu_i[j] == MULT);
      fp_event[j] = commit_ack_i[j] & (commit_fu_i[j] == FPU || commit_fu_i[j] == FPU_VEC);
    end

    for (int unsigned i = 1; i <= MHPMCounterNum; i++) begin
      case (mhpmevent_q[i])
        5'b00000: events[i] = 0;
        5'b00001: events[i] = l1_icache_miss_i;
        5'b00010: events[i] = l1_dcache_miss_i;
        5'b00011: events[i] = itlb_miss_i;
        5'b00100: events[i] = dtlb_miss_i;
        5'b00101: events[i] = |load_event;
        5'b00110: events[i] = |store_event;
        5'b00111: events[i] = ex_valid_i;
        5'b01000: events[i] = eret_i;
        5'b01001: events[i] = |branch_event;
        5'b01010: events[i] = resolved_branch_valid_i && resolved_branch_is_mispredict_i;
        5'b01011: events[i] = branch_exception_valid_i;
        5'b01100: events[i] = |call_event;
        5'b01101: events[i] = |return_event;
        5'b01110: events[i] = sb_full_i;
        5'b01111: events[i] = if_empty_i;
        5'b10000: events[i] = l1_icache_req_i;
        5'b10001: events[i] = l1_dcache_req_i[0] || l1_dcache_req_i[1] || l1_dcache_req_i[2];
        5'b10010:
          events[i] = (l1_dcache_miss_i && miss_vld_bits_i[0] == 8'hFF) ||
                      (l1_dcache_miss_i && miss_vld_bits_i[1] == 8'hFF) ||
                      (l1_dcache_miss_i && miss_vld_bits_i[2] == 8'hFF);
        5'b10011: events[i] = i_tlb_flush_i;
        5'b10100: events[i] = |int_event;
        5'b10101: events[i] = |fp_event;
        5'b10110: events[i] = stall_issue_i;
        default: events[i] = 0;
      endcase
    end
  end

  always_comb begin : generic_counter
    generic_counter_d = generic_counter_q;
    data_o = 'b0;
    mhpmevent_d = mhpmevent_q;
    read_access_exception = 1'b0;
    update_access_exception = 1'b0;

    for (int unsigned i = 1; i <= MHPMCounterNum; i++) begin
      if ((!debug_mode_i) && (!we_i)) begin
        // Guard the exact CVA6 perf-counter increment shape, including the
        // explicit single-bit equality that has shown up as an xnor gate in the
        // failing SEC memory-boundary report.
        if ((events[i]) == 1 && (!mcountinhibit_i[i+2])) begin
          generic_counter_d[i] = generic_counter_q[i] + 1'b1;
        end
      end
    end

    if ((addr_i >= csr_addr_t'(CSR_MHPM_COUNTER_3)) &&
        (addr_i < (csr_addr_t'(CSR_MHPM_COUNTER_3) + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3+1][31:0];
      end else begin
        data_o = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3+1];
      end
    end else if ((addr_i >= csr_addr_t'(CSR_MHPM_COUNTER_3H)) &&
                 (addr_i < (csr_addr_t'(CSR_MHPM_COUNTER_3H) + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3H+1][63:32];
      end else begin
        read_access_exception = 1'b1;
      end
    end else if ((addr_i >= csr_addr_t'(CSR_MHPM_EVENT_3)) &&
                 (addr_i < (csr_addr_t'(CSR_MHPM_EVENT_3) + MHPMCounterNum))) begin
      data_o = mhpmevent_q[addr_i-CSR_MHPM_EVENT_3+1];
    end else if ((addr_i >= csr_addr_t'(CSR_HPM_COUNTER_3)) &&
                 (addr_i < (csr_addr_t'(CSR_HPM_COUNTER_3) + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o = generic_counter_q[addr_i-CSR_HPM_COUNTER_3+1][31:0];
      end else begin
        data_o = generic_counter_q[addr_i-CSR_HPM_COUNTER_3+1];
      end
    end else if ((addr_i > csr_addr_t'(CSR_HPM_COUNTER_3H)) &&
                 (addr_i < (csr_addr_t'(CSR_HPM_COUNTER_3H) + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3H+1][63:32];
      end else begin
        read_access_exception = 1'b1;
      end
    end

    if (we_i) begin
      if ((addr_i >= csr_addr_t'(CSR_MHPM_COUNTER_3)) &&
          (addr_i < (csr_addr_t'(CSR_MHPM_COUNTER_3) + MHPMCounterNum))) begin
        if (XLEN == 32) begin
          generic_counter_d[addr_i-CSR_MHPM_COUNTER_3+1][31:0] = data_i;
        end else begin
          generic_counter_d[addr_i-CSR_MHPM_COUNTER_3+1] = data_i;
        end
      end else if ((addr_i >= csr_addr_t'(CSR_MHPM_COUNTER_3H)) &&
                   (addr_i < (csr_addr_t'(CSR_MHPM_COUNTER_3H) + MHPMCounterNum))) begin
        if (XLEN == 32) begin
          generic_counter_d[addr_i-CSR_MHPM_COUNTER_3H+1][63:32] = data_i;
        end else begin
          update_access_exception = 1'b1;
        end
      end else if ((addr_i >= csr_addr_t'(CSR_MHPM_EVENT_3)) &&
                   (addr_i < csr_addr_t'(CSR_MHPM_EVENT_3) + MHPMCounterNum)) begin
        mhpmevent_d[addr_i-CSR_MHPM_EVENT_3+1] = data_i;
      end
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      generic_counter_q <= '{default: 0};
      mhpmevent_q <= '{default: 0};
    end else begin
      generic_counter_q <= generic_counter_d;
      mhpmevent_q <= mhpmevent_d;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName(
      "near_literal_cva6_perf_counters_memory_inference_keeps_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);

  // Guard the exact CVA6 perf-counter regression that later surfaced in SEC:
  // inferred memory dependency pins such as WE/RADDR/WDATA must have a
  // supported combinational cone all the way back to top/seq/const terms, not
  // just "no globally dangling input terms" in the flattened netlist.
  const auto boundaryIssues = collectUnsupportedMemoryBoundaryTerms(top);
  EXPECT_TRUE(boundaryIssues.empty()) << formatStringVector(boundaryIssues);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseRealCva6PerfCountersModuleKeepsStructuredMemoryDependenciesSupported) {
  const auto context = resolveCva6SourceContext();
  if (!context.has_value()) {
    GTEST_SKIP() << "CVA6 source tree is not available for the real-source memory regression test";
  }

  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
      outPath /
      "real_cva6_perf_counters_module_keeps_structured_memory_dependencies_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto wrapperPath =
      outPath /
      "real_cva6_perf_counters_module_keeps_structured_memory_dependencies_supported.sv";
  std::ofstream wrapperFile(wrapperPath);
  ASSERT_TRUE(wrapperFile.good());
  wrapperFile
      << R"(module real_cva6_perf_counters_module_keeps_structured_memory_dependencies_supported
  import ariane_pkg::*;
#(
  parameter config_pkg::cva6_cfg_t CVA6Cfg = config_pkg::cva6_cfg_empty
) ();
  localparam type branchpredict_sbe_t = struct packed {
    cf_t                     cf;
    logic [CVA6Cfg.VLEN-1:0] predict_address;
  };

  localparam type exception_t = struct packed {
    logic [CVA6Cfg.XLEN-1:0]  cause;
    logic [CVA6Cfg.XLEN-1:0]  tval;
    logic [CVA6Cfg.GPLEN-1:0] tval2;
    logic [31:0]              tinst;
    logic                     gva;
    logic                     valid;
  };

  localparam type bp_resolve_t = struct packed {
    logic                    valid;
    logic [CVA6Cfg.VLEN-1:0] pc;
    logic [CVA6Cfg.VLEN-1:0] target_address;
    logic                    is_mispredict;
    logic                    is_taken;
    cf_t                     cf_type;
  };

  localparam type icache_dreq_t = struct packed {
    logic                    req;
    logic                    kill_s1;
    logic                    kill_s2;
    logic                    spec;
    logic [CVA6Cfg.VLEN-1:0] vaddr;
  };

  localparam type cbo_t = logic [7:0];

  localparam type dcache_req_i_t = struct packed {
    logic [CVA6Cfg.DCACHE_INDEX_WIDTH-1:0] address_index;
    logic [CVA6Cfg.DCACHE_TAG_WIDTH-1:0]   address_tag;
    logic [CVA6Cfg.XLEN-1:0]               data_wdata;
    logic [CVA6Cfg.DCACHE_USER_WIDTH-1:0]  data_wuser;
    logic                                  data_req;
    logic                                  data_we;
    logic [(CVA6Cfg.XLEN/8)-1:0]           data_be;
    logic [1:0]                            data_size;
    logic [CVA6Cfg.DcacheIdWidth-1:0]      data_id;
    logic                                  kill_req;
    logic                                  tag_valid;
    cbo_t                                  cbo_op;
  };

  localparam type scoreboard_entry_t = struct packed {
    logic [CVA6Cfg.VLEN-1:0]              pc;
    logic [CVA6Cfg.TRANS_ID_BITS-1:0]     trans_id;
    fu_t                                  fu;
    fu_op                                 op;
    logic [REG_ADDR_SIZE-1:0]             rs1;
    logic [REG_ADDR_SIZE-1:0]             rs2;
    logic [REG_ADDR_SIZE-1:0]             rd;
    logic [CVA6Cfg.XLEN-1:0]              result;
    logic                                 valid;
    logic                                 use_imm;
    logic                                 use_zimm;
    logic                                 use_pc;
    exception_t                           ex;
    branchpredict_sbe_t                   bp;
    logic                                 is_compressed;
    logic                                 is_macro_instr;
    logic                                 is_last_macro_instr;
    logic                                 is_double_rd_macro_instr;
    logic                                 vfp;
    logic                                 is_zcmt;
  };

  logic clk_i;
  logic rst_ni;
  logic debug_mode_i;
  logic [11:0] addr_i;
  logic we_i;
  logic [CVA6Cfg.XLEN-1:0] data_i;
  logic [CVA6Cfg.XLEN-1:0] data_o;
  scoreboard_entry_t [CVA6Cfg.NrCommitPorts-1:0] commit_instr_i;
  logic [CVA6Cfg.NrCommitPorts-1:0] commit_ack_i;
  logic l1_icache_miss_i;
  logic l1_dcache_miss_i;
  logic itlb_miss_i;
  logic dtlb_miss_i;
  logic sb_full_i;
  logic if_empty_i;
  exception_t ex_i;
  logic eret_i;
  bp_resolve_t resolved_branch_i;
  exception_t branch_exceptions_i;
  icache_dreq_t l1_icache_access_i;
  dcache_req_i_t [2:0] l1_dcache_access_i;
  logic [2:0][CVA6Cfg.DCACHE_SET_ASSOC-1:0] miss_vld_bits_i;
  logic i_tlb_flush_i;
  logic stall_issue_i;
  logic [31:0] mcountinhibit_i;

  perf_counters #(
    .CVA6Cfg(CVA6Cfg),
    .bp_resolve_t(bp_resolve_t),
    .dcache_req_i_t(dcache_req_i_t),
    .dcache_req_o_t(dcache_req_i_t),
    .exception_t(exception_t),
    .icache_dreq_t(icache_dreq_t),
    .scoreboard_entry_t(scoreboard_entry_t)
  ) dut (
    .clk_i(clk_i),
    .rst_ni(rst_ni),
    .debug_mode_i(debug_mode_i),
    .addr_i(addr_i),
    .we_i(we_i),
    .data_i(data_i),
    .data_o(data_o),
    .commit_instr_i(commit_instr_i),
    .commit_ack_i(commit_ack_i),
    .l1_icache_miss_i(l1_icache_miss_i),
    .l1_dcache_miss_i(l1_dcache_miss_i),
    .itlb_miss_i(itlb_miss_i),
    .dtlb_miss_i(dtlb_miss_i),
    .sb_full_i(sb_full_i),
    .if_empty_i(if_empty_i),
    .ex_i(ex_i),
    .eret_i(eret_i),
    .resolved_branch_i(resolved_branch_i),
    .branch_exceptions_i(branch_exceptions_i),
    .l1_icache_access_i(l1_icache_access_i),
    .l1_dcache_access_i(l1_dcache_access_i),
    .miss_vld_bits_i(miss_vld_bits_i),
    .i_tlb_flush_i(i_tlb_flush_i),
    .stall_issue_i(stall_issue_i),
    .mcountinhibit_i(mcountinhibit_i)
  );
endmodule
)";
  wrapperFile.close();

  SNLSVConstructor constructor(library_);
  const auto args = buildExpandedCva6SlangArgs(
      *context,
      "real_cva6_perf_counters_module_keeps_structured_memory_dependencies_supported",
      {wrapperPath});
  constructor.construct(args);

  auto* top = library_->getSNLDesign(
      NLName("real_cva6_perf_counters_module_keeps_structured_memory_dependencies_supported"));
  ASSERT_NE(top, nullptr);

  // Guard the real CVA6 perf-counter source rather than a handwritten model:
  // every inferred-memory dependency bit exposed by the actual RTL module must
  // trace through a supported combinational cone instead of later surfacing as
  // a skipped `structured memory dependency` in KF extraction.
  const auto boundaryIssues = collectUnsupportedMemoryBoundaryTerms(top);
  EXPECT_TRUE(boundaryIssues.empty()) << formatStringVector(boundaryIssues);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseRealCva6PerfCountersModuleWithTargetConfigKeepsStructuredMemoryDependenciesSupported) {
  const auto context = resolveCva6SourceContext();
  if (!context.has_value()) {
    GTEST_SKIP() << "CVA6 source tree is not available for the real-source memory regression test";
  }

  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
      outPath /
      "real_cva6_perf_counters_module_with_target_config_keeps_structured_memory_dependencies_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto wrapperPath =
      outPath /
      "real_cva6_perf_counters_module_with_target_config_keeps_structured_memory_dependencies_supported.sv";
  std::ofstream wrapperFile(wrapperPath);
  ASSERT_TRUE(wrapperFile.good());
  wrapperFile
      << R"(module real_cva6_perf_counters_module_with_target_config_keeps_structured_memory_dependencies_supported
  import ariane_pkg::*;
  import cva6_config_pkg::*;
#(
  parameter config_pkg::cva6_cfg_t CVA6Cfg =
      build_config_pkg::build_config(cva6_cfg)
) ();
  localparam type branchpredict_sbe_t = struct packed {
    cf_t                     cf;
    logic [CVA6Cfg.VLEN-1:0] predict_address;
  };

  localparam type exception_t = struct packed {
    logic [CVA6Cfg.XLEN-1:0]  cause;
    logic [CVA6Cfg.XLEN-1:0]  tval;
    logic [CVA6Cfg.GPLEN-1:0] tval2;
    logic [31:0]              tinst;
    logic                     gva;
    logic                     valid;
  };

  localparam type bp_resolve_t = struct packed {
    logic                    valid;
    logic [CVA6Cfg.VLEN-1:0] pc;
    logic [CVA6Cfg.VLEN-1:0] target_address;
    logic                    is_mispredict;
    logic                    is_taken;
    cf_t                     cf_type;
  };

  localparam type icache_dreq_t = struct packed {
    logic                    req;
    logic                    kill_s1;
    logic                    kill_s2;
    logic                    spec;
    logic [CVA6Cfg.VLEN-1:0] vaddr;
  };

  localparam type cbo_t = logic [7:0];

  localparam type dcache_req_i_t = struct packed {
    logic [CVA6Cfg.DCACHE_INDEX_WIDTH-1:0] address_index;
    logic [CVA6Cfg.DCACHE_TAG_WIDTH-1:0]   address_tag;
    logic [CVA6Cfg.XLEN-1:0]               data_wdata;
    logic [CVA6Cfg.DCACHE_USER_WIDTH-1:0]  data_wuser;
    logic                                  data_req;
    logic                                  data_we;
    logic [(CVA6Cfg.XLEN/8)-1:0]           data_be;
    logic [1:0]                            data_size;
    logic [CVA6Cfg.DcacheIdWidth-1:0]      data_id;
    logic                                  kill_req;
    logic                                  tag_valid;
    cbo_t                                  cbo_op;
  };

  localparam type scoreboard_entry_t = struct packed {
    logic [CVA6Cfg.VLEN-1:0]              pc;
    logic [CVA6Cfg.TRANS_ID_BITS-1:0]     trans_id;
    fu_t                                  fu;
    fu_op                                 op;
    logic [REG_ADDR_SIZE-1:0]             rs1;
    logic [REG_ADDR_SIZE-1:0]             rs2;
    logic [REG_ADDR_SIZE-1:0]             rd;
    logic [CVA6Cfg.XLEN-1:0]              result;
    logic                                 valid;
    logic                                 use_imm;
    logic                                 use_zimm;
    logic                                 use_pc;
    exception_t                           ex;
    branchpredict_sbe_t                   bp;
    logic                                 is_compressed;
    logic                                 is_macro_instr;
    logic                                 is_last_macro_instr;
    logic                                 is_double_rd_macro_instr;
    logic                                 vfp;
    logic                                 is_zcmt;
  };

  logic clk_i;
  logic rst_ni;
  logic debug_mode_i;
  logic [11:0] addr_i;
  logic we_i;
  logic [CVA6Cfg.XLEN-1:0] data_i;
  logic [CVA6Cfg.XLEN-1:0] data_o;
  scoreboard_entry_t [CVA6Cfg.NrCommitPorts-1:0] commit_instr_i;
  logic [CVA6Cfg.NrCommitPorts-1:0] commit_ack_i;
  logic l1_icache_miss_i;
  logic l1_dcache_miss_i;
  logic itlb_miss_i;
  logic dtlb_miss_i;
  logic sb_full_i;
  logic if_empty_i;
  exception_t ex_i;
  logic eret_i;
  bp_resolve_t resolved_branch_i;
  exception_t branch_exceptions_i;
  icache_dreq_t l1_icache_access_i;
  dcache_req_i_t [2:0] l1_dcache_access_i;
  logic [2:0][CVA6Cfg.DCACHE_SET_ASSOC-1:0] miss_vld_bits_i;
  logic i_tlb_flush_i;
  logic stall_issue_i;
  logic [31:0] mcountinhibit_i;

  perf_counters #(
    .CVA6Cfg(CVA6Cfg),
    .bp_resolve_t(bp_resolve_t),
    .dcache_req_i_t(dcache_req_i_t),
    .dcache_req_o_t(dcache_req_i_t),
    .exception_t(exception_t),
    .icache_dreq_t(icache_dreq_t),
    .scoreboard_entry_t(scoreboard_entry_t)
  ) dut (
    .clk_i(clk_i),
    .rst_ni(rst_ni),
    .debug_mode_i(debug_mode_i),
    .addr_i(addr_i),
    .we_i(we_i),
    .data_i(data_i),
    .data_o(data_o),
    .commit_instr_i(commit_instr_i),
    .commit_ack_i(commit_ack_i),
    .l1_icache_miss_i(l1_icache_miss_i),
    .l1_dcache_miss_i(l1_dcache_miss_i),
    .itlb_miss_i(itlb_miss_i),
    .dtlb_miss_i(dtlb_miss_i),
    .sb_full_i(sb_full_i),
    .if_empty_i(if_empty_i),
    .ex_i(ex_i),
    .eret_i(eret_i),
    .resolved_branch_i(resolved_branch_i),
    .branch_exceptions_i(branch_exceptions_i),
    .l1_icache_access_i(l1_icache_access_i),
    .l1_dcache_access_i(l1_dcache_access_i),
    .miss_vld_bits_i(miss_vld_bits_i),
    .i_tlb_flush_i(i_tlb_flush_i),
    .stall_issue_i(stall_issue_i),
    .mcountinhibit_i(mcountinhibit_i)
  );
endmodule
)";
  wrapperFile.close();

  SNLSVConstructor constructor(library_);
  const auto args = buildExpandedCva6SlangArgs(
      *context,
      "real_cva6_perf_counters_module_with_target_config_keeps_structured_memory_dependencies_supported",
      {wrapperPath});
  constructor.construct(args);

  auto* top = library_->getSNLDesign(
      NLName("real_cva6_perf_counters_module_with_target_config_keeps_structured_memory_dependencies_supported"));
  ASSERT_NE(top, nullptr);

  // Guard the exact configured CVA6 perf-counter path that later shows up in
  // KF as `generic_counter_q_mem.WE[1]`: the inferred-memory boundary exposed
  // by the real target config must stay driven all the way back to supported
  // combinational roots.
  const auto boundaryIssues = collectUnsupportedMemoryBoundaryTerms(top);
  EXPECT_TRUE(boundaryIssues.empty()) << formatStringVector(boundaryIssues);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseRealCva6TopKeepsPerfCounterMemoryWriteDataBoundarySupported) {
  const auto context = resolveCva6SourceContext();
  if (!context.has_value()) {
    GTEST_SKIP() << "CVA6 source tree is not available for the real-source memory regression test";
  }

  SNLSVConstructor constructor(library_);
  const auto args = buildExpandedCva6SlangArgs(*context, "cva6");
  constructor.construct(args);

  auto* top = library_->getSNLDesign(NLName("cva6"));
  ASSERT_NE(top, nullptr);

  const auto boundaryIssues = collectUnsupportedMemoryBoundaryTerms(top);
  std::vector<std::string> perfCounterWriteDataIssues;
  for (const auto& issue : boundaryIssues) {
    // Guard the exact full-CVA6 regression currently surfacing in KF as a
    // logical-loop skip while tracing the inferred perf-counter memory write
    // data boundary. This path should lower to a supported cone back to
    // top/seq/const terms, not a self-referential mux chain.
    if (issue.find("generic_counter_q_mem.WDATA[384]") != std::string::npos) {
      perfCounterWriteDataIssues.push_back(issue);
    }
  }
  EXPECT_TRUE(perfCounterWriteDataIssues.empty())
      << formatStringVector(perfCounterWriteDataIssues);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseRealCva6TopDoesNotCreateTransparentMuxSelfReferenceOnPerfCounterWriteDataPath) {
  const auto context = resolveCva6SourceContext();
  if (!context.has_value()) {
    GTEST_SKIP() << "CVA6 source tree is not available for the real-source memory regression test";
  }

  SNLSVConstructor constructor(library_);
  const auto args = buildExpandedCva6SlangArgs(*context, "cva6");
  constructor.construct(args);

  auto* top = library_->getSNLDesign(NLName("cva6"));
  ASSERT_NE(top, nullptr);

  const auto offenders = collectTransparentCombinationalSelfReferences(
    top,
    {"csr_regfile_i.32255."});
  std::vector<std::string> perfCounterMuxSelfReferences;
  for (const auto& offender : offenders) {
    // Guard the exact CVA6 perf-counter lowering bug currently reproduced in
    // KF as a logical-loop skip on generic_counter_q_mem.WDATA[384]: no mux
    // input in that cone should resolve back to the same mux output through
    // transparent assign aliases.
    if (offender.find("csr_regfile_i.32255.") != std::string::npos ||
        offender.find("generic_counter_q_mem") != std::string::npos) {
      perfCounterMuxSelfReferences.push_back(offender);
    }
  }

  EXPECT_TRUE(perfCounterMuxSelfReferences.empty())
      << formatStringVector(perfCounterMuxSelfReferences);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parsePerfCounterLikeArrayUpdateDoesNotCreateTransparentMuxSelfReference) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "perf_counter_like_array_update_no_transparent_mux_self_reference";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "perf_counter_like_array_update_no_transparent_mux_self_reference.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module perf_counter_like_array_update_no_transparent_mux_self_reference(
  input  logic        clk_i,
  input  logic        rst_ni,
  input  logic        we_i,
  input  logic [1:0]  addr_i,
  input  logic [63:0] data_i,
  output logic [63:0] data_o
);
  logic [63:0] counter_d [3:1];
  logic [63:0] counter_q [3:1];

  always_comb begin
    counter_d = counter_q;
    for (int unsigned i = 1; i <= 3; i++) begin
      if (!we_i) begin
        counter_d[i] = counter_q[i] + 1'b1;
      end
    end
    if (we_i) begin
      counter_d[addr_i + 1][31:0] = data_i[31:0];
    end
    data_o = counter_q[addr_i + 1];
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      counter_q <= '{default: 0};
    end else begin
      counter_q <= counter_d;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("perf_counter_like_array_update_no_transparent_mux_self_reference"));
  ASSERT_NE(top, nullptr);

  const auto offenders = collectTransparentCombinationalSelfReferences(top);
  EXPECT_TRUE(offenders.empty()) << formatStringVector(offenders);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseNearLiteralCva6PerfCountersStructPortMemoryInferenceKeepsInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath /
    "near_literal_cva6_perf_counters_struct_port_memory_inference_keeps_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "near_literal_cva6_perf_counters_struct_port_memory_inference_keeps_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module near_literal_cva6_perf_counters_struct_port_memory_inference_keeps_internal_inputs_driven(
  input logic clk_i,
  input logic rst_ni,
  input logic debug_mode_i,
  input logic [11:0] addr_i,
  input logic we_i,
  input logic [63:0] data_i,
  output logic [63:0] data_o,
  input logic [31:0] mcountinhibit_i,
  input logic l1_icache_miss_i,
  input logic l1_dcache_miss_i,
  input logic itlb_miss_i,
  input logic dtlb_miss_i,
  input logic sb_full_i,
  input logic if_empty_i,
  input logic eret_i,
  input logic i_tlb_flush_i,
  input logic stall_issue_i,
  input logic [2:0][7:0] miss_vld_bits_i,
  input logic [1:0] commit_ack_i
);
  localparam int unsigned XLEN = 64;
  localparam int unsigned MHPMCounterNum = 6;
  localparam int unsigned NrCommitPorts = 2;
  localparam int unsigned NumPorts = 3;
  localparam int unsigned DCACHE_SET_ASSOC = 8;

  localparam logic [3:0] LOAD = 4'd1;
  localparam logic [3:0] STORE = 4'd2;
  localparam logic [3:0] CTRL_FLOW = 4'd3;
  localparam logic [3:0] ALU = 4'd4;
  localparam logic [3:0] MULT = 4'd5;
  localparam logic [3:0] FPU = 4'd6;
  localparam logic [3:0] FPU_VEC = 4'd7;
  localparam logic [3:0] ADD = 4'd8;
  localparam logic [3:0] JALR = 4'd9;

  typedef logic [11:0] csr_addr_t;
  localparam csr_addr_t CSR_MHPM_COUNTER_3 = 12'hB03;
  localparam csr_addr_t CSR_MHPM_COUNTER_3H = 12'hB83;
  localparam csr_addr_t CSR_MHPM_EVENT_3 = 12'h323;
  localparam csr_addr_t CSR_HPM_COUNTER_3 = 12'hC03;
  localparam csr_addr_t CSR_HPM_COUNTER_3H = 12'hC83;

  typedef struct packed {
    logic [3:0] fu;
    logic [3:0] op;
    logic [4:0] rd;
  } scoreboard_entry_t;

  typedef struct packed {
    logic valid;
  } exception_t;

  typedef struct packed {
    logic valid;
    logic is_mispredict;
  } bp_resolve_t;

  typedef struct packed {
    logic req;
  } icache_dreq_t;

  typedef struct packed {
    logic data_req;
  } dcache_req_i_t;

  scoreboard_entry_t [NrCommitPorts-1:0] commit_instr_i;
  exception_t ex_i;
  exception_t branch_exceptions_i;
  bp_resolve_t resolved_branch_i;
  icache_dreq_t l1_icache_access_i;
  dcache_req_i_t [2:0] l1_dcache_access_i;

  logic [63:0] generic_counter_d[MHPMCounterNum:1];
  logic [63:0] generic_counter_q[MHPMCounterNum:1];
  logic read_access_exception, update_access_exception;
  logic events[MHPMCounterNum:1];
  logic [4:0] mhpmevent_d[MHPMCounterNum:1];
  logic [4:0] mhpmevent_q[MHPMCounterNum:1];
  logic [NrCommitPorts-1:0] load_event;
  logic [NrCommitPorts-1:0] store_event;
  logic [NrCommitPorts-1:0] branch_event;
  logic [NrCommitPorts-1:0] call_event;
  logic [NrCommitPorts-1:0] return_event;
  logic [NrCommitPorts-1:0] int_event;
  logic [NrCommitPorts-1:0] fp_event;

  always_comb begin
    for (int unsigned j = 0; j < NrCommitPorts; j++) begin
      commit_instr_i[j].fu = (j == 0) ? LOAD : CTRL_FLOW;
      commit_instr_i[j].op = (j == 0) ? ADD : JALR;
      commit_instr_i[j].rd = (j == 0) ? 5'd1 : 5'd0;
    end
    ex_i.valid = l1_dcache_miss_i;
    branch_exceptions_i.valid = dtlb_miss_i;
    resolved_branch_i.valid = if_empty_i;
    resolved_branch_i.is_mispredict = sb_full_i;
    l1_icache_access_i.req = l1_icache_miss_i;
    for (int unsigned i = 0; i < NumPorts; i++) begin
      l1_dcache_access_i[i].data_req = miss_vld_bits_i[i][0];
    end
  end

  always_comb begin : Mux
    events[MHPMCounterNum:1] = '{default: 0};
    load_event = '{default: 0};
    store_event = '{default: 0};
    branch_event = '{default: 0};
    call_event = '{default: 0};
    return_event = '{default: 0};
    int_event = '{default: 0};
    fp_event = '{default: 0};

    for (int unsigned j = 0; j < NrCommitPorts; j++) begin
      load_event[j] = commit_ack_i[j] & (commit_instr_i[j].fu == LOAD);
      store_event[j] = commit_ack_i[j] & (commit_instr_i[j].fu == STORE);
      branch_event[j] = commit_ack_i[j] & (commit_instr_i[j].fu == CTRL_FLOW);
      call_event[j] = commit_ack_i[j] & (commit_instr_i[j].fu == CTRL_FLOW &&
                                         (commit_instr_i[j].op == ADD || commit_instr_i[j].op == JALR) &&
                                         (commit_instr_i[j].rd == 5'd1 || commit_instr_i[j].rd == 5'd5));
      return_event[j] = commit_ack_i[j] & (commit_instr_i[j].op == JALR && commit_instr_i[j].rd == 5'd0);
      int_event[j] = commit_ack_i[j] & (commit_instr_i[j].fu == ALU || commit_instr_i[j].fu == MULT);
      fp_event[j] = commit_ack_i[j] & (commit_instr_i[j].fu == FPU || commit_instr_i[j].fu == FPU_VEC);
    end

    for (int unsigned i = 1; i <= MHPMCounterNum; i++) begin
      case (mhpmevent_q[i])
        5'b00000: events[i] = 0;
        5'b00001: events[i] = l1_icache_miss_i;
        5'b00010: events[i] = l1_dcache_miss_i;
        5'b00011: events[i] = itlb_miss_i;
        5'b00100: events[i] = dtlb_miss_i;
        5'b00101: events[i] = |load_event;
        5'b00110: events[i] = |store_event;
        5'b00111: events[i] = ex_i.valid;
        5'b01000: events[i] = eret_i;
        5'b01001: events[i] = |branch_event;
        5'b01010: events[i] = resolved_branch_i.valid && resolved_branch_i.is_mispredict;
        5'b01011: events[i] = branch_exceptions_i.valid;
        5'b01100: events[i] = |call_event;
        5'b01101: events[i] = |return_event;
        5'b01110: events[i] = sb_full_i;
        5'b01111: events[i] = if_empty_i;
        5'b10000: events[i] = l1_icache_access_i.req;
        5'b10001: events[i] = l1_dcache_access_i[0].data_req ||
                              l1_dcache_access_i[1].data_req ||
                              l1_dcache_access_i[2].data_req;
        5'b10010:
          events[i] = (l1_dcache_miss_i && miss_vld_bits_i[0] == 8'hFF) ||
                      (l1_dcache_miss_i && miss_vld_bits_i[1] == 8'hFF) ||
                      (l1_dcache_miss_i && miss_vld_bits_i[2] == 8'hFF);
        5'b10011: events[i] = i_tlb_flush_i;
        5'b10100: events[i] = |int_event;
        5'b10101: events[i] = |fp_event;
        5'b10110: events[i] = stall_issue_i;
        default: events[i] = 0;
      endcase
    end
  end

  always_comb begin : generic_counter
    generic_counter_d = generic_counter_q;
    data_o = 'b0;
    mhpmevent_d = mhpmevent_q;
    read_access_exception = 1'b0;
    update_access_exception = 1'b0;

    for (int unsigned i = 1; i <= MHPMCounterNum; i++) begin
      if ((!debug_mode_i) && (!we_i)) begin
        // Guard the full CVA6 perf-counter surface, including struct-field
        // event muxing and the single-bit equality compare that has shown up in
        // SEC as an undriven xnor input on inferred-memory write enables.
        if ((events[i]) == 1 && (!mcountinhibit_i[i+2])) begin
          generic_counter_d[i] = generic_counter_q[i] + 1'b1;
        end
      end
    end

    if ((addr_i >= csr_addr_t'(CSR_MHPM_COUNTER_3)) &&
        (addr_i < (csr_addr_t'(CSR_MHPM_COUNTER_3) + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3+1][31:0];
      end else begin
        data_o = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3+1];
      end
    end else if ((addr_i >= csr_addr_t'(CSR_MHPM_COUNTER_3H)) &&
                 (addr_i < (csr_addr_t'(CSR_MHPM_COUNTER_3H) + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3H+1][63:32];
      end else begin
        read_access_exception = 1'b1;
      end
    end else if ((addr_i >= csr_addr_t'(CSR_MHPM_EVENT_3)) &&
                 (addr_i < (csr_addr_t'(CSR_MHPM_EVENT_3) + MHPMCounterNum))) begin
      data_o = mhpmevent_q[addr_i-CSR_MHPM_EVENT_3+1];
    end else if ((addr_i >= csr_addr_t'(CSR_HPM_COUNTER_3)) &&
                 (addr_i < (csr_addr_t'(CSR_HPM_COUNTER_3) + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o = generic_counter_q[addr_i-CSR_HPM_COUNTER_3+1][31:0];
      end else begin
        data_o = generic_counter_q[addr_i-CSR_HPM_COUNTER_3+1];
      end
    end else if ((addr_i > csr_addr_t'(CSR_HPM_COUNTER_3H)) &&
                 (addr_i < (csr_addr_t'(CSR_HPM_COUNTER_3H) + MHPMCounterNum))) begin
      if (XLEN == 32) begin
        data_o = generic_counter_q[addr_i-CSR_MHPM_COUNTER_3H+1][63:32];
      end else begin
        read_access_exception = 1'b1;
      end
    end

    if (we_i) begin
      if ((addr_i >= csr_addr_t'(CSR_MHPM_COUNTER_3)) &&
          (addr_i < (csr_addr_t'(CSR_MHPM_COUNTER_3) + MHPMCounterNum))) begin
        if (XLEN == 32) begin
          generic_counter_d[addr_i-CSR_MHPM_COUNTER_3+1][31:0] = data_i;
        end else begin
          generic_counter_d[addr_i-CSR_MHPM_COUNTER_3+1] = data_i;
        end
      end else if ((addr_i >= csr_addr_t'(CSR_MHPM_COUNTER_3H)) &&
                   (addr_i < (csr_addr_t'(CSR_MHPM_COUNTER_3H) + MHPMCounterNum))) begin
        if (XLEN == 32) begin
          generic_counter_d[addr_i-CSR_MHPM_COUNTER_3H+1][63:32] = data_i;
        end else begin
          update_access_exception = 1'b1;
        end
      end else if ((addr_i >= csr_addr_t'(CSR_MHPM_EVENT_3)) &&
                   (addr_i < csr_addr_t'(CSR_MHPM_EVENT_3) + MHPMCounterNum)) begin
        mhpmevent_d[addr_i-CSR_MHPM_EVENT_3+1] = data_i;
      end
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      generic_counter_q <= '{default: 0};
      mhpmevent_q <= '{default: 0};
    end else begin
      generic_counter_q <= generic_counter_d;
      mhpmevent_q <= mhpmevent_d;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName(
      "near_literal_cva6_perf_counters_struct_port_memory_inference_keeps_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseCva6StoreBufferStyleMemoryInferenceKeepsInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "cva6_store_buffer_style_memory_inference_keeps_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "cva6_store_buffer_style_memory_inference_keeps_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module cva6_store_buffer_style_memory_inference_keeps_internal_inputs_driven(
  input  logic        clk_i,
  input  logic        rst_ni,
  input  logic        commit_i,
  input  logic        valid_i,
  input  logic        stall_st_pending_i,
  input  logic        data_gnt_i,
  input  logic        data_rvalid_i,
  input  logic [63:0] paddr_i,
  input  logic [63:0] data_i,
  input  logic [7:0]  be_i,
  input  logic [1:0]  data_size_i,
  input  logic [1:0]  speculative_read_pointer_q,
  input  logic [1:0]  speculative_write_pointer_q,
  input  logic [1:0]  commit_read_pointer_q,
  input  logic [1:0]  commit_write_pointer_q,
  output logic [63:0] req_data_o
);
  localparam int unsigned DEPTH = 4;
  typedef struct packed {
    logic [63:0] address;
    logic [63:0] data;
    logic [7:0]  be;
    logic [1:0]  data_size;
    logic        valid;
    logic        wait_rvalid;
  } entry_t;

  entry_t speculative_queue_n[DEPTH-1:0], speculative_queue_q[DEPTH-1:0];
  entry_t commit_queue_n[DEPTH-1:0],      commit_queue_q[DEPTH-1:0];

  always_comb begin : core_if
    speculative_queue_n = speculative_queue_q;
    if (valid_i) begin
      speculative_queue_n[speculative_write_pointer_q].address = paddr_i;
      speculative_queue_n[speculative_write_pointer_q].data = data_i;
      speculative_queue_n[speculative_write_pointer_q].be = be_i;
      speculative_queue_n[speculative_write_pointer_q].data_size = data_size_i;
      speculative_queue_n[speculative_write_pointer_q].valid = 1'b1;
      speculative_queue_n[speculative_write_pointer_q].wait_rvalid = 1'b0;
    end
    if (commit_i) begin
      speculative_queue_n[speculative_read_pointer_q].valid = 1'b0;
    end
  end

  always_comb begin : store_if
    commit_queue_n = commit_queue_q;

    if (commit_queue_q[commit_read_pointer_q].valid &&
        !stall_st_pending_i &&
        !commit_queue_q[commit_read_pointer_q].wait_rvalid) begin
      if (data_gnt_i) begin
        if (data_rvalid_i) begin
          commit_queue_n[commit_read_pointer_q].valid = 1'b0;
        end else begin
          commit_queue_n[commit_read_pointer_q].wait_rvalid = 1'b1;
        end
      end
    end

    if (commit_queue_q[commit_read_pointer_q].valid &&
        commit_queue_q[commit_read_pointer_q].wait_rvalid &&
        data_rvalid_i) begin
      commit_queue_n[commit_read_pointer_q].valid = 1'b0;
    end

    if (commit_i) begin
      commit_queue_n[commit_write_pointer_q] =
        speculative_queue_q[speculative_read_pointer_q];
    end
  end

  assign req_data_o = commit_queue_q[commit_read_pointer_q].data;

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      speculative_queue_q <= '{default: 0};
      commit_queue_q <= '{default: 0};
    end else begin
      speculative_queue_q <= speculative_queue_n;
      commit_queue_q <= commit_queue_n;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("cva6_store_buffer_style_memory_inference_keeps_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  // Guard the CVA6 store-buffer regression: packed-struct memory entries that
  // are copied wholesale between queues and then partially updated must not
  // leave internal write-data mux inputs undriven.
  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parsePerfCounterStyleWriteOnlyMemoryInferenceKeepsInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "perf_counter_style_write_only_memory_inference_keeps_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
      outPath / "perf_counter_style_write_only_memory_inference_keeps_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module perf_counter_style_write_only_memory_inference_keeps_internal_inputs_driven(
  input  logic        clk_i,
  input  logic        rst_ni,
  input  logic [11:0] addr_i,
  input  logic        we_i,
  input  logic [31:0] data_i,
  output logic        flag_o
);
  localparam int unsigned CounterNum = 8;
  localparam logic [11:0] CounterBase = 12'hB03;

  logic [63:0] generic_counter_d[CounterNum:1];
  logic [63:0] generic_counter_q[CounterNum:1];

  always_comb begin
    flag_o = we_i;
    generic_counter_d = generic_counter_q;

    for (int unsigned i = 1; i <= CounterNum; i++) begin
      generic_counter_d[i] = generic_counter_q[i] + 1'b1;
    end

    if (we_i) begin
      if ((addr_i >= CounterBase) && (addr_i < (CounterBase + CounterNum))) begin
        generic_counter_d[addr_i - CounterBase + 1][31:0] = data_i;
      end
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      generic_counter_q <= '{default: 0};
    end else begin
      generic_counter_q <= generic_counter_d;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
      NLName("perf_counter_style_write_only_memory_inference_keeps_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  size_t memoryCount = 0;
  for (auto* inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ++memoryCount;
    }
  }
  EXPECT_EQ(1u, memoryCount);

  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parsePerfCounterStyleReadOnlyMemoryInferenceKeepsInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "perf_counter_style_read_only_memory_inference_keeps_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
      outPath / "perf_counter_style_read_only_memory_inference_keeps_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module perf_counter_style_read_only_memory_inference_keeps_internal_inputs_driven(
  input  logic        clk_i,
  input  logic        rst_ni,
  input  logic [11:0] addr_i,
  output logic [63:0] data_o
);
  localparam int unsigned CounterNum = 8;
  localparam logic [11:0] CounterBase = 12'hB03;

  logic [63:0] generic_counter_d[CounterNum:1];
  logic [63:0] generic_counter_q[CounterNum:1];

  always_comb begin
    generic_counter_d = generic_counter_q;
    data_o = '0;

    if ((addr_i >= CounterBase) && (addr_i < (CounterBase + CounterNum))) begin
      data_o = generic_counter_q[addr_i - CounterBase + 1];
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      generic_counter_q <= '{default: 0};
    end else begin
      generic_counter_q <= generic_counter_d;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
      NLName("perf_counter_style_read_only_memory_inference_keeps_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  size_t memoryCount = 0;
  for (auto* inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ++memoryCount;
    }
  }
  EXPECT_EQ(0u, memoryCount);

  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseArithmeticDynamicElementSelectKeepsInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "arithmetic_dynamic_element_select_keeps_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
      outPath / "arithmetic_dynamic_element_select_keeps_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module arithmetic_dynamic_element_select_keeps_internal_inputs_driven(
  input  logic [11:0] addr_i,
  input  logic [31:0] data_i,
  output logic        flag_o
);
  localparam int unsigned CounterNum = 8;
  localparam logic [11:0] CounterBase = 12'hB03;

  logic [63:0] state_q[CounterNum:1];
  logic [63:0] state_d[CounterNum:1];

  always_comb begin
    flag_o = 1'b0;
    state_d = '{default: 64'h0};
    state_d[addr_i - CounterBase + 1][31:0] = data_i;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
      NLName("arithmetic_dynamic_element_select_keeps_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceDirectArithmeticPartialWriteKeepsInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_direct_arithmetic_partial_write_keeps_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
      outPath / "qd_memory_inference_direct_arithmetic_partial_write_keeps_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_direct_arithmetic_partial_write_keeps_internal_inputs_driven(
  input  logic        clk_i,
  input  logic [11:0] addr_i,
  input  logic [31:0] data_i,
  output logic [63:0] data_o
);
  logic [63:0] mem_q [0:63];
  logic [63:0] mem_d [0:63];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i - 12'hB03 + 12'd1][31:0] = data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
      NLName("qd_memory_inference_direct_arithmetic_partial_write_keeps_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  size_t memoryCount = 0;
  for (auto* inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ++memoryCount;
    }
  }
  EXPECT_EQ(1u, memoryCount);

  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceLoopIndexWritesKeepInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_loop_index_writes_keep_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
      outPath / "qd_memory_inference_loop_index_writes_keep_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_loop_index_writes_keep_internal_inputs_driven(
  input  logic        clk_i,
  output logic [63:0] data_o
);
  localparam int unsigned CounterNum = 8;

  logic [63:0] mem_q[CounterNum:1];
  logic [63:0] mem_d[CounterNum:1];

  always_comb begin
    mem_d = mem_q;
    for (int unsigned i = 1; i <= CounterNum; i++) begin
      mem_d[i] = mem_q[i] + 1'b1;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[1];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
      NLName("qd_memory_inference_loop_index_writes_keep_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  size_t memoryCount = 0;
  for (auto* inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ++memoryCount;
    }
  }
  EXPECT_EQ(1u, memoryCount);

  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceDynamicSelfReadWriteKeepsInternalInputsDriven) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_dynamic_self_read_write_keeps_internal_inputs_driven";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
      outPath / "qd_memory_inference_dynamic_self_read_write_keeps_internal_inputs_driven.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_dynamic_self_read_write_keeps_internal_inputs_driven(
  input  logic       clk_i,
  input  logic [2:0] addr_i,
  output logic [63:0] data_o
);
  logic [63:0] mem_q [0:7];
  logic [63:0] mem_d [0:7];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = mem_q[addr_i] + 1'b1;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
      NLName("qd_memory_inference_dynamic_self_read_write_keeps_internal_inputs_driven"));
  ASSERT_NE(top, nullptr);

  size_t memoryCount = 0;
  for (auto* inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ++memoryCount;
    }
  }
  EXPECT_EQ(1u, memoryCount);

  const auto danglingInputs = collectDanglingInternalInputTerms(top);
  EXPECT_TRUE(danglingInputs.empty()) << formatStringVector(danglingInputs);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIndexedCommitConstantFalseGuardSkipSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_constant_false_guard_skip_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_constant_false_guard_skip_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_constant_false_guard_skip_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (i == 0) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_next_indexed_commit_constant_false_guard_skip_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIndexedCommitLateConstantFalseGuardSkipSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_late_constant_false_guard_skip_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_next_indexed_commit_late_constant_false_guard_skip_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_late_constant_false_guard_skip_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i][0] && 1'b0) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_next_indexed_commit_late_constant_false_guard_skip_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIndexedCommitConstantSelectorFastPathsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_constant_selector_fast_paths_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_constant_selector_fast_paths_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_constant_selector_fast_paths_supported(
  input  logic       clk_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[1] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (mem_d[i][0]) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[1];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_next_indexed_commit_constant_selector_fast_paths_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIndexedCommitSelectorMismatchFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_selector_mismatch_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_selector_mismatch_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_selector_mismatch_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (mem_d[0][0]) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIndexedCommitRhsSelectorMismatchFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_rhs_selector_mismatch_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_rhs_selector_mismatch_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_rhs_selector_mismatch_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[0];
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextDynamicIndexedCommitTargetFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_dynamic_indexed_commit_target_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_dynamic_indexed_commit_target_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_dynamic_indexed_commit_target_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] commit_idx_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[commit_idx_i] = mem_d[commit_idx_i];
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported dynamic indexed commit target";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIfTrueIndexedCommitFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_iftrue_indexed_commit_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_iftrue_indexed_commit_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_iftrue_indexed_commit_failure_fallback(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (sel_i) begin
        mem_next[i] += mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported indexed commit action in ifTrue branch";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIfFalseIndexedCommitFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_iffalse_indexed_commit_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_iffalse_indexed_commit_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_iffalse_indexed_commit_failure_fallback(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (sel_i) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] += mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported indexed commit action in ifFalse branch";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIgnoresEmptyAndUnrelatedCommitCandidates) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_ignores_empty_and_unrelated_commit_candidates";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_ignores_empty_and_unrelated_commit_candidates.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_ignores_empty_and_unrelated_commit_candidates(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic       scratch;
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    ;
  end

  always_comb begin
    scratch = addr_i[0];
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (i < 2) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_next_ignores_empty_and_unrelated_commit_candidates"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIgnoresNonAssignmentCommitCandidate) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_ignores_non_assignment_commit_candidate";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_ignores_non_assignment_commit_candidate.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_ignores_non_assignment_commit_candidate(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic       scratch;
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    case (addr_i)
      default: ;
    endcase
  end

  always_comb begin
    scratch = addr_i[0];
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (i < 2) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_next_ignores_non_assignment_commit_candidate"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextCompoundCommitBlockFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_compound_commit_block_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_compound_commit_block_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_compound_commit_block_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next[addr_i] += data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextSelfCopyCommitBlockFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_self_copy_commit_block_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_self_copy_commit_block_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_self_copy_commit_block_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_next;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextLocalCommitFieldFallbackSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_field_fallback_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_field_fallback_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_field_fallback_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] access_i,
  input  logic [3:0] payload_i,
  output logic [7:0] data_o
);
  typedef struct packed {
    logic [1:0] mode;
    logic [1:0] access;
    logic [3:0] payload;
  } entry_t;

  entry_t mem_q [0:3];
  entry_t mem_d [0:3];
  entry_t mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i].mode = mode_i;
    mem_d[addr_i].access = access_i;
    mem_d[addr_i].payload = payload_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i].mode == 2'b11) begin
        mem_next[i].mode = mem_q[i].mode;
      end
      if (mem_d[i].access == 2'b01) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = {mem_q[addr_i].mode, mem_q[addr_i].access, mem_q[addr_i].payload};
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_local_commit_field_fallback_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, wrPortsParam);
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("3", wrPortsParam->getValue());
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferencePartialFieldWritesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_partial_field_writes_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_partial_field_writes_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_partial_field_writes_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [5:0] payload_i,
  output logic [5:0] payload_o,
  output logic [1:0] reserved_o
);
  typedef struct packed {
    logic [5:0] payload;
    logic [1:0] reserved;
  } entry_t;

  entry_t mem_q [0:3];
  entry_t mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i].reserved = 2'b00;
    mem_d[addr_i].payload = payload_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign payload_o = mem_q[addr_i].payload;
  assign reserved_o = mem_q[addr_i].reserved;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_partial_field_writes_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, wrPortsParam);
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("2", wrPortsParam->getValue());
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferencePartialSliceWritesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_partial_slice_writes_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_partial_slice_writes_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_partial_slice_writes_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [3:0] upper_i,
  input  logic [1:0] lower_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][7:4] = upper_i;
    mem_d[addr_i][1:0] = lower_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_partial_slice_writes_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("2", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferencePackedElementWriteSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_packed_element_write_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_packed_element_write_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_packed_element_write_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic       bit_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][3] = bit_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_packed_element_write_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceDynamicPackedElementWriteFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_dynamic_packed_element_write_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_dynamic_packed_element_write_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_dynamic_packed_element_write_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [2:0] bit_sel_i,
  input  logic       bit_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][bit_sel_i] = bit_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported dynamic packed element shadow write";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceOutOfRangePackedElementWriteFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_out_of_range_packed_element_write_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_out_of_range_packed_element_write_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_out_of_range_packed_element_write_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic       bit_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][8] = bit_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported out-of-range packed element shadow write";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceIndexedUpRangeWriteSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_indexed_up_range_write_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_indexed_up_range_write_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_indexed_up_range_write_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] slice_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][2 +: 2] = slice_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_memory_inference_indexed_up_range_write_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextLocalCommitLogicalOrSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_logical_or_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_logical_or_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_logical_or_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] access_i,
  input  logic [3:0] payload_i,
  output logic [7:0] data_o
);
  typedef struct packed {
    logic [1:0] mode;
    logic [1:0] access;
    logic [3:0] payload;
  } entry_t;

  entry_t mem_q [0:3];
  entry_t mem_d [0:3];
  entry_t mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i].mode = mode_i;
    mem_d[addr_i].access = access_i;
    mem_d[addr_i].payload = payload_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if ((mem_d[i].mode == 2'b11) || (mem_d[i].access == 2'b01)) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = {mem_q[addr_i].mode, mem_q[addr_i].access, mem_q[addr_i].payload};
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_local_commit_logical_or_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("3", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextLocalCommitSingleBitEqualitySupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_single_bit_equality_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_single_bit_equality_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_single_bit_equality_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic       bit_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][0] = bit_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i][0] == mem_q[i][0]) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_local_commit_single_bit_equality_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextLocalCommitSingleBitInequalitySupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_single_bit_inequality_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_single_bit_inequality_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_single_bit_inequality_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic       bit_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][0] = bit_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i][0] != mem_q[i][0]) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_next_local_commit_single_bit_inequality_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextLocalCommitEqualityRhsFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_equality_rhs_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_equality_rhs_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_equality_rhs_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic       bit_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][0] = bit_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i][0] == mem_d[0][0]) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextLocalCommitLogicalAndSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_logical_and_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_logical_and_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_logical_and_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i][0] && mem_q[i][0]) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_local_commit_logical_and_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextLocalCommitLogicalAndLhsFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_logical_and_lhs_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_logical_and_lhs_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_logical_and_lhs_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[0][0] && mem_d[i][0]) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextLocalCommitLogicalAndRhsFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_logical_and_rhs_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_logical_and_rhs_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_logical_and_rhs_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i][0] && mem_d[0][0]) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIndexedCommitUnaryLocalConditionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_unary_local_condition_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_unary_local_condition_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_unary_local_condition_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (!mem_d[i][0]) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_next_indexed_commit_unary_local_condition_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDNextIndexedCommitUnaryConstantLocalConditionShortcutsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_unary_constant_local_condition_shortcuts";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_next_indexed_commit_unary_constant_local_condition_shortcuts.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_unary_constant_local_condition_shortcuts(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_q[i];
      if (!(mem_d[i][0] && 1'b0)) begin
        mem_next[i] = mem_d[i];
      end
      if (!(mem_d[i][0] || 1'b1)) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_next_indexed_commit_unary_constant_local_condition_shortcuts"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceIndexedDownRangeWriteSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_indexed_down_range_write_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_indexed_down_range_write_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_indexed_down_range_write_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] slice_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][5 -: 2] = slice_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_memory_inference_indexed_down_range_write_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceDynamicIndexedRangeWriteFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_dynamic_indexed_range_write_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_dynamic_indexed_range_write_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_dynamic_indexed_range_write_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [2:0] start_i,
  input  logic [1:0] slice_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][start_i +: 2] = slice_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported dynamic indexed range shadow write";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceCurrentEntryBitsSelectorFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_current_entry_bits_selector_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_current_entry_bits_selector_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_current_entry_bits_selector_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [3:0] slice_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[$urandom_range(3, 0)][3:0] = slice_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported current-entry selector lowering";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceOutOfRangeSimpleRangeWriteFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_out_of_range_simple_range_write_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_out_of_range_simple_range_write_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_out_of_range_simple_range_write_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] slice_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][9:8] = slice_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported out-of-range simple range shadow write";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceWholeEntryWriteDataBitsFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_whole_entry_write_data_bits_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_whole_entry_write_data_bits_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_whole_entry_write_data_bits_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = $urandom_range(8'hFF, 8'h00);
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferencePartialWriteAssignBitsFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_partial_write_assign_bits_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_partial_write_assign_bits_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_partial_write_assign_bits_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][3:0] = $urandom_range(4'hF, 4'h0);
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceDuplicateDynamicSelectorPartialWritesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_duplicate_dynamic_selector_partial_writes_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_duplicate_dynamic_selector_partial_writes_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_duplicate_dynamic_selector_partial_writes_supported(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [3:0] lo_i,
  input  logic [3:0] hi_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      mem_d[addr_i][3:0] = lo_i;
      mem_d[addr_i][7:4] = hi_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_duplicate_dynamic_selector_partial_writes_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceDisjointConstantPartialWritesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_disjoint_constant_partial_writes_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_disjoint_constant_partial_writes_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_disjoint_constant_partial_writes_supported(
  input  logic       clk_i,
  input  logic       sel_lo_i,
  input  logic       sel_hi_i,
  input  logic [3:0] lo_i,
  input  logic [3:0] hi_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_lo_i) begin
      mem_d[0][3:0] = lo_i;
    end
    if (sel_hi_i) begin
      mem_d[1][7:4] = hi_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_disjoint_constant_partial_writes_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSameConstantPartialWritesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_same_constant_partial_writes_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_same_constant_partial_writes_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_same_constant_partial_writes_supported(
  input  logic       clk_i,
  input  logic [3:0] lo_i,
  input  logic [3:0] hi_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[0][3:0] = lo_i;
    mem_d[0][7:4] = hi_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_same_constant_partial_writes_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSymbolicConstantPartialWritesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_symbolic_constant_partial_writes_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_symbolic_constant_partial_writes_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_symbolic_constant_partial_writes_supported(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic       en_i,
  input  logic [3:0] lo_i,
  input  logic [3:0] hi_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      mem_d[0][3:0] = lo_i;
    end
    if (en_i) begin
      mem_d[0][7:4] = hi_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_symbolic_constant_partial_writes_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceNestedDuplicateGuardSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_nested_duplicate_guard_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_nested_duplicate_guard_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_nested_duplicate_guard_supported(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      if (sel_i) begin
        mem_d[addr_i] = data_i;
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_nested_duplicate_guard_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceCaseGuardConstantTrueShortcutSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_case_guard_constant_true_shortcut_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_case_guard_constant_true_shortcut_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_case_guard_constant_true_shortcut_supported(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      case (1'b0)
        1'b0: mem_d[addr_i] = data_i;
        default: mem_d[addr_i] = 8'h00;
      endcase
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_case_guard_constant_true_shortcut_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceUntimedSequentialCandidateIgnored) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_untimed_sequential_candidate_ignored";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_untimed_sequential_candidate_ignored.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_untimed_sequential_candidate_ignored(
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported statement while extracting sequential timing control",
     "unable to resolve a single-bit clock net"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSequentialNedgeClockFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_sequential_nedge_clock_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_sequential_nedge_clock_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_sequential_nedge_clock_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always @(negedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSequentialClockNetFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_sequential_clock_net_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_sequential_clock_net_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_sequential_clock_net_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always @(posedge (clk_i + 2'd1)) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unable to resolve a single-bit clock net"});
}
