// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include <filesystem>
#include <cstdlib>
#include "SNLVRLDumper.h"
#include "NetlistGraph.h"
#include <fstream>
#include <functional>
#include <sstream>

#include "gtest/gtest.h"
#include "NLUniverse.h"
#include "NLDB0.h"
#include "NLDB.h"
#include "NLLibrary.h"
#include "SNLDesign.h"
#include "SNLBusTerm.h"
#include "SNLDesignModeling.h"
#include "SNLInstance.h"
#include "SNLInstParameter.h"
#include "SNLInstTerm.h"
#include "SNLParameter.h"
#include "SNLScalarNet.h"
#include "SNLCapnP.h"

using namespace naja::NL;

namespace {
using Modeling = SNLDesignModeling;
struct PrimitiveCase {
  std::string name;
  std::function<SNLDesign*()> create;
};

std::vector<PrimitiveCase> primitiveCases(bool wide = true) {
  std::vector<PrimitiveCase> cases{{"assign", NLDB0::getAssign}, {"fa", NLDB0::getFA}};
  const std::vector<std::pair<std::string, SNLDesign*(*)(size_t)>> sized{
    {"mux", NLDB0::getOrCreateMux2}, {"latch", NLDB0::getOrCreateDLatch},
    {"dff", NLDB0::getOrCreateDFF}, {"dffn", NLDB0::getOrCreateDFFN},
    {"dffrn", NLDB0::getOrCreateDFFRN}, {"dffr", NLDB0::getOrCreateDFFR},
    {"dffs", NLDB0::getOrCreateDFFS}, {"dffe", NLDB0::getOrCreateDFFE},
    {"dffre", NLDB0::getOrCreateDFFRE}, {"dffse", NLDB0::getOrCreateDFFSE},
    {"dffsr", NLDB0::getOrCreateDFFSR}, {"dffsrn", NLDB0::getOrCreateDFFSRN},
    {"dffss", NLDB0::getOrCreateDFFSS}, {"dffssn", NLDB0::getOrCreateDFFSSN},
    {"dffsre", NLDB0::getOrCreateDFFSRE}, {"dffsrne", NLDB0::getOrCreateDFFSRNE},
    {"dffsse", NLDB0::getOrCreateDFFSSE}, {"dffssne", NLDB0::getOrCreateDFFSSNE},
    {"multiply", NLDB0::getOrCreateMultiply}};
  for (size_t width : {1u, 5u, 65u}) {
    if (!wide && width == 65) continue;
    for (const auto& [name, create] : sized)
      cases.push_back({name + std::to_string(width), [=] { return create(width); }});
    for (bool subtract : {false, true})
      cases.push_back({std::string(subtract ? "subtract" : "add") + std::to_string(width),
          [=] { return NLDB0::getOrCreateAddSub(width, subtract); }});
    for (bool isSigned : {false, true})
      cases.push_back({std::string(isSigned ? "signed_divmod" : "unsigned_divmod") + std::to_string(width),
          [=] { return NLDB0::getOrCreateDivMod({width, isSigned}); }});
    for (auto type : {NLDB0::GateType::And, NLDB0::GateType::Nand, NLDB0::GateType::Or,
        NLDB0::GateType::Nor, NLDB0::GateType::Xor, NLDB0::GateType::Xnor,
        NLDB0::GateType::Buf, NLDB0::GateType::Not}) {
      NLDB0::GateType gate(type);
      cases.push_back({gate.getString() + std::to_string(width), [=] {
        return gate.isNInput() ? NLDB0::getOrCreateNInputGate(gate, width)
            : NLDB0::getOrCreateNOutputGate(gate, width);
      }});
    }
    for (size_t countWidth : {1u, 80u}) {
      for (int mode = 0; mode < 3; ++mode)
        cases.push_back({"shift" + std::to_string(width) + "c" + std::to_string(countWidth) + "m" + std::to_string(mode),
            [=] { return NLDB0::getOrCreateShift({width, countWidth, mode != 0, mode == 2}); }});
    }
    cases.push_back({"table" + std::to_string(width),
        [=] { return NLDB0::getOrCreateTableSelect({width, 3, 2}); }});
    for (auto reset : {NLDB0::MemoryResetMode::None, NLDB0::MemoryResetMode::AsyncLow,
        NLDB0::MemoryResetMode::AsyncHigh, NLDB0::MemoryResetMode::SyncLow,
        NLDB0::MemoryResetMode::SyncHigh})
      cases.push_back({"memory" + std::to_string(width) + "r" + std::to_string(int(reset)),
          [=] { return NLDB0::getOrCreateMemory({width, 3, 2, 2, 2, reset}); }});
    cases.push_back({"rom" + std::to_string(width),
        [=] { return NLDB0::getOrCreateMemory({width, 3, 2, 2, 0}); }});
    cases.push_back({"masked" + std::to_string(width),
        [=] { return NLDB0::getOrCreateMemory({width, 3, 2, 2, 2, NLDB0::MemoryResetMode::None, true}); }});
  }
  return cases;
}

void expression(std::ostream& out, const Modeling::BooleanExpression& expression) {
  out << "expr " << expression.root << '\n';
  for (const auto& node : expression.nodes) {
    out << int(node.operation) << ' ' << node.constant << ' ' << node.state << ' '
        << (node.term ? node.term->getNLID().getString() : "null");
    for (auto operand : node.operands) out << ' ' << operand;
    out << '\n';
  }
}

// Compare live IDs and modeling before any dumper can impose an ordering.
std::string state(SNLDesign* top) {
  std::ostringstream out;
  for (auto* instance : top->getInstances()) {
    auto* model = instance->getModel();
    out << instance->getNLID().getString() << ' ' << instance->getName().getString()
        << ' ' << model->getNLID().getString() << ' ' << model->getName().getString() << '\n';
    for (auto* parameter : model->getParameters()) {
      auto* override = instance->getInstParameter(parameter->getName());
      out << parameter->getName().getString() << ' ' << parameter->getType().getString()
          << ' ' << parameter->getValue() << ' ' << (override ? override->getValue() : "inherited") << '\n';
    }
    for (auto* term : model->getTerms())
      out << term->getNLID().getString() << ' ' << term->getName().getString() << ' '
          << term->getDirection().getString() << ' ' << term->getWidth() << '\n';
    for (auto* term : instance->getInstTerms()) {
      out << term->getNLID().getString() << ' ' << term->getNet()->getNLID().getString()
          << ' ' << term->getNet()->getName().getString() << ' '
          << int(Modeling::getTermRole(term)) << ' ' << int(Modeling::getResetActiveLevel(term)) << '\n';
      using Query = naja::NajaCollection<SNLInstTerm*>(*)(SNLInstTerm*);
      for (Query query : std::initializer_list<Query>{Modeling::getCombinatorialInputs, Modeling::getCombinatorialOutputs,
          Modeling::getInputRelatedClocks, Modeling::getOutputRelatedClocks,
          Modeling::getClockRelatedInputs, Modeling::getClockRelatedOutputs}) {
        for (auto* related : query(term)) out << related->getNLID().getString() << ' ';
        out << '\n';
      }
    }
    if (NLDB0::isNInputGate(model) ||
        (NLDB0::isNOutputGate(model) && NLDB0::getGateNTerms(model)->getWidth() == 1) ||
        NLDB0::isAssign(model) || NLDB0::isMux2(model)) {
      const auto truth = NLDB0::getPrimitiveTruthTable(model);
      out << truth.toString() << ' ' << truth.isGeneric() << ' '
          << truth.isAnd() << truth.isNand() << truth.isOr() << truth.isNor()
          << truth.isXor() << truth.isXnor() << truth.isMux() << '\n';
    }
    if (NLDB0::isFA(model)) {
      out << NLDB0::getFASumTruthTable().toString() << '\n'
          << NLDB0::getFACoutTruthTable().toString() << '\n';
    }
    if (NLDB0::isDivMod(model) || NLDB0::isTableSelect(model)) {
      for (auto* term : model->getBitTerms())
        if (term->getDirection() == SNLTerm::Direction::Output)
          out << Modeling::getTruthTable(instance, term->getOrderID()).toString() << '\n';
    }
    if (Modeling::hasSequentialModel(model)) {
      const auto& sequential = Modeling::getSequentialModel(instance);
      out << "sequential " << int(sequential.kind) << '\n';
      expression(out, sequential.clockedOn);
      for (const auto& item : sequential.states) {
        expression(out, item.nextState);
        out << bool(item.clear) << ' ' << bool(item.preset) << ' ' << int(item.clearPresetValue) << '\n';
        if (item.clear) expression(out, *item.clear);
        if (item.preset) expression(out, *item.preset);
      }
      for (const auto& item : sequential.outputs) {
        out << item.term->getNLID().getString() << '\n';
        expression(out, item.function);
      }
    }
    if (NLDB0::isMemory(model)) {
      auto memory = Modeling::getMemoryInterface(instance);
      out << "memory " << memory.width << ' ' << memory.depth << ' ' << memory.abits << ' ' << int(memory.resetMode) << '\n';
      const auto term = [&](auto* bit) { out << (bit ? bit->getNLID().getString() : "null") << ' '; };
      term(memory.clock); term(memory.reset);
      for (const auto& port : memory.readPorts) {
        out << "read ";
        for (auto bits : {port.address, port.data, port.enables}) {
          for (auto* bit : bits) term(bit);
          out << '\n';
        }
      }
      for (const auto& port : memory.writePorts) {
        out << "write "; term(port.clock);
        for (auto bits : {port.address, port.data, port.mask, port.enables}) {
          for (auto* bit : bits) term(bit);
          out << '\n';
        }
      }
    }
  }
  return out.str();
}

std::string bytes(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

class SNLCapnPPrimitiveAuditTest : public ::testing::TestWithParam<PrimitiveCase> {
 protected:
  void roundTrip(bool explicitOnly);
  void TearDown() override {
    if (NLUniverse::get()) NLUniverse::get()->destroy();
  }
};

void SNLCapnPPrimitiveAuditTest::roundTrip(bool explicitOnly) {
  auto* universe = NLUniverse::create();
  auto* db = NLDB::create(universe);
  auto* top = SNLDesign::create(NLLibrary::create(db, NLName("designs")), NLName("top"));
  universe->setTopDesign(top);
  auto* model = GetParam().create();
  // All three share a canonical model: defaults, explicit defaults and data overrides.
  for (int mode = 0; mode < 3; ++mode) {
    auto* instance = SNLInstance::create(top, model, NLName("u" + std::to_string(mode)));
    for (auto* parameter : model->getParameters()) {
      if (mode == 1 || (explicitOnly && (mode != 2 ||
          (parameter->getName() != NLName("INIT") && parameter->getName() != NLName("INIT_ENABLE")))))
        SNLInstParameter::create(instance, parameter, parameter->getValue());
      if (mode == 2 && parameter->getName() == NLName("INIT"))
        SNLInstParameter::create(instance, parameter, "9'b10xz01101");
      if (mode == 2 && parameter->getName() == NLName("INIT_ENABLE"))
        SNLInstParameter::create(instance, parameter, "1");
    }
    for (auto* term : instance->getInstTerms())
      term->setNet(SNLScalarNet::create(top, NLName("n" + std::to_string(top->getNets().size()))));
  }
  const auto expected = state(top);
  const auto directory = std::filesystem::path(SNL_CAPNP_TEST_PATH) / ("primitive_audit_" + GetParam().name + (explicitOnly ? "_explicit" : "_inherited"));
  std::filesystem::create_directories(directory);
  SNLCapnP::dump(db, directory / "first");
  universe->destroy();
  ASSERT_NO_THROW(db = SNLCapnP::load(directory / "first"));
  top = db->getTopDesign();
  EXPECT_EQ(expected, state(top));
  EXPECT_EQ(GetParam().create(), top->getInstance(NLName("u0"))->getModel());
  EXPECT_EQ(top->getInstance(NLName("u0"))->getModel(), top->getInstance(NLName("u2"))->getModel());
  SNLCapnP::dump(db, directory / "second");
  for (const auto& file : {"snl.mf", "db0_interface.snl", "db_interface.snl", "db_implementation.snl"})
    EXPECT_TRUE(bytes(directory / "first" / file) == bytes(directory / "second" / file)) << file;
}

TEST_P(SNLCapnPPrimitiveAuditTest, InheritedRoundTrip) { roundTrip(false); }
TEST_P(SNLCapnPPrimitiveAuditTest, ExplicitRoundTrip) { roundTrip(true); }

class SNLCapnPPrimitiveIdentityTest : public ::testing::Test {
 protected:
  void TearDown() override {
    if (NLUniverse::get()) NLUniverse::get()->destroy();
  }
  SNLDesign* createTop() {
    auto* universe = NLUniverse::create();
    auto* top = SNLDesign::create(NLLibrary::create(NLDB::create(universe), NLName("designs")), NLName("top"));
    universe->setTopDesign(top);
    return top;
  }
  void roundTrip(SNLDesign* top, const std::string& name) {
    const auto expected = state(top);
    const auto directory = std::filesystem::path(SNL_CAPNP_TEST_PATH) / name;
    std::filesystem::create_directories(directory);
    SNLCapnP::dump(top->getDB(), directory / "first");
    NLUniverse::get()->destroy();
    auto* db = SNLCapnP::load(directory / "first");
    EXPECT_EQ(expected, state(db->getTopDesign()));
    SNLCapnP::dump(db, directory / "second");
    for (const auto& file : {"snl.mf", "db0_interface.snl", "db_interface.snl", "db_implementation.snl"})
      EXPECT_TRUE(bytes(directory / "first" / file) == bytes(directory / "second" / file)) << file;
  }
  void add(SNLDesign* top, SNLDesign* model, const char* name, bool resetOverride = false, bool initialize = false) {
    auto* instance = SNLInstance::create(top, model, NLName(name));
    for (auto* parameter : model->getParameters()) {
      auto value = parameter->getValue();
      if (resetOverride && parameter->getName() == NLName("RST_ENABLE")) value = "1";
      if (initialize && parameter->getName() == NLName("INIT")) value = "9'b10xz01101";
      if (initialize && parameter->getName() == NLName("INIT_ENABLE")) value = "1";
      SNLInstParameter::create(instance, parameter, value);
    }
    for (auto* term : instance->getInstTerms())
      term->setNet(SNLScalarNet::create(top, NLName("n" + std::to_string(top->getNets().size()))));
  }
};

TEST_F(SNLCapnPPrimitiveIdentityTest, MemoryOverridePreservesModelAndSharing) {
  auto* top = createTop();
  auto* model = NLDB0::getOrCreateMemory({3, 3, 2, 1, 1});
  add(top, model, "overridden", true);
  add(top, model, "defaulted");
  roundTrip(top, "memory_override_identity");
  top = NLUniverse::get()->getDB(1)->getTopDesign();
  EXPECT_EQ(top->getInstance(NLName("overridden"))->getModel(),
      top->getInstance(NLName("defaulted"))->getModel());
}

TEST_F(SNLCapnPPrimitiveIdentityTest, CompoundModelsInstantiatedInReverseOrder) {
  auto* top = createTop();
  auto* first = NLDB0::getOrCreateShift({3, 2, false, false});
  auto* second = NLDB0::getOrCreateShift({5, 3, true, true});
  add(top, second, "second");
  add(top, first, "first");
  roundTrip(top, "compound_reverse_identity");
}

TEST_F(SNLCapnPPrimitiveIdentityTest, UnusedCompoundModelPrecedesUsedModel) {
  auto* top = createTop();
  NLDB0::getOrCreateTableSelect({2, 3, 2});
  add(top, NLDB0::getOrCreateTableSelect({5, 4, 2}), "used");
  roundTrip(top, "compound_gap_identity");
}

// Python's determinism suite invokes just this test in independent writer and
// reader processes. Its ordinary GTest execution also checks a fresh universe.
TEST_F(SNLCapnPPrimitiveIdentityTest, AllFamiliesArtifacts) {
  const char* output = std::getenv("NAJA_DB0_AUDIT_OUTPUT");
  const char* input = std::getenv("NAJA_DB0_AUDIT_INPUT");
  const auto directory = output ? std::filesystem::path(output)
      : std::filesystem::path(SNL_CAPNP_TEST_PATH) / "all_families_artifacts";
  const auto emit = [](SNLDesign* top, const std::filesystem::path& path) {
    std::filesystem::create_directories(path);
    std::ofstream(path / "state.txt") << state(top);
    SNLVRLDumper dumper;
    dumper.setSingleFile(true);
    dumper.setTopFileName("design.v");
    dumper.dumpDesign(top, path);
    naja::SnlVisualiser visualiser(top, true);
    visualiser.process();
    visualiser.getNetlistGraph().dumpDotFile((path / "design.dot").string().c_str());
    SNLCapnP::dump(top->getDB(), path / "snapshot");
  };
  if (input) {
    auto* db = SNLCapnP::load(input);
    emit(db->getTopDesign(), directory);
    return;
  }
  // Perturb native allocations across writer processes, retaining them until
  // after reload. ASLR independently perturbs all reader processes.
  const char* run = std::getenv("NAJA_DB0_AUDIT_RUN");
  std::vector<std::vector<char>> allocations;
  for (int i = 0; i < 128 + (run ? std::atoi(run) * 17 : 0); ++i)
    allocations.emplace_back(512 + (i * 137) % 8192);
  auto* top = createTop();
  NLDB0::getOrCreateTableSelect({2, 7, 3}); // An unused model must retain its ID.
  std::vector<std::pair<std::string, SNLDesign*>> models;
  for (const auto& item : primitiveCases(false)) models.emplace_back(item.name, item.create());
  // Model creation order deliberately differs from first instance use.
  for (auto it = models.rbegin(); it != models.rend(); ++it) {
    auto* inherited = SNLInstance::create(top, it->second, NLName(it->first + "_inherited"));
    for (auto* term : inherited->getInstTerms())
      term->setNet(SNLScalarNet::create(top, NLName("n" + std::to_string(top->getNets().size()))));
    add(top, it->second, (it->first + "_explicit").c_str());
    if (it->second->getParameter(NLName("INIT")))
      add(top, it->second, (it->first + "_initialized").c_str(), false, true);
  }
  auto* resetModel = NLDB0::getOrCreateMemory({3, 3, 2, 1, 1});
  add(top, resetModel, "reset_overridden", true, true);
  add(top, resetModel, "reset_defaulted");
  const auto expected = state(top);
  emit(top, directory / "original");
  NLUniverse::get()->destroy();
  auto* db = SNLCapnP::load(directory / "original" / "snapshot");
  EXPECT_EQ(expected, state(db->getTopDesign()));
  emit(db->getTopDesign(), directory / "reloaded");
}

TEST_F(SNLCapnPPrimitiveIdentityTest, MissingDB0DefinitionsAreRejected) {
  auto* top = createTop();
  const auto path = std::filesystem::path(SNL_CAPNP_TEST_PATH) / "missing_db0_definitions";
  SNLCapnP::dump(top->getDB(), path);
  std::filesystem::remove(path / SNLCapnP::DB0InterfaceName);
  NLUniverse::get()->destroy();
  EXPECT_THROW(SNLCapnP::load(path), NLException);
  EXPECT_EQ(nullptr, NLUniverse::get());
}

TEST_F(SNLCapnPPrimitiveIdentityTest, ConflictingExistingDB0IdentityIsRejected) {
  auto* top = createTop();
  add(top, NLDB0::getOrCreateShift({3, 2, false, false}), "used");
  const auto path = std::filesystem::path(SNL_CAPNP_TEST_PATH) / "conflicting_db0_identity";
  SNLCapnP::dump(top->getDB(), path);
  NLUniverse::get()->destroy();
  NLUniverse::create();
  NLDB0::getOrCreateShift({7, 4, true, false});
  EXPECT_THROW(SNLCapnP::load(path), NLException);
  EXPECT_TRUE(NLUniverse::get()->getUserDBs().empty());
}

TEST_F(SNLCapnPPrimitiveIdentityTest, SeparateInterfaceAndImplementationRoundTrip) {
  auto* top = createTop();
  add(top, NLDB0::getOrCreateMemory({3, 3, 2, 1, 1}), "used", true, true);
  const auto expected = state(top);
  const auto path = std::filesystem::path(SNL_CAPNP_TEST_PATH) / "separate_db0_payloads";
  std::filesystem::create_directories(path);
  SNLCapnP::dumpInterface(NLDB0::getDB0(), path / "db0");
  SNLCapnP::dumpInterface(top->getDB(), path / "interface");
  SNLCapnP::dumpImplementation(top->getDB(), path / "implementation");
  NLUniverse::get()->destroy();
  SNLCapnP::LoadingConfiguration configuration;
  SNLCapnP::loadInterface(path / "db0", configuration);
  SNLCapnP::loadInterface(path / "interface", configuration);
  auto* db = SNLCapnP::loadImplementation(path / "implementation", configuration);
  EXPECT_EQ(expected, state(db->getTopDesign()));
}

TEST_F(SNLCapnPPrimitiveIdentityTest, IncompleteModelSignatureIsRejected) {
  auto* top = createTop();
  auto* model = NLDB0::getOrCreateShift({3, 2, false, false});
  add(top, model, "used");
  model->getParameter(NLName("COUNT_WIDTH"))->destroy();
  const auto path = std::filesystem::path(SNL_CAPNP_TEST_PATH) / "incomplete_db0_signature";
  SNLCapnP::dump(top->getDB(), path);
  NLUniverse::get()->destroy();
  EXPECT_THROW(SNLCapnP::load(path), NLException);
}

TEST_F(SNLCapnPPrimitiveIdentityTest, ModifiedCanonicalInterfaceIsRejected) {
  auto* top = createTop();
  auto* model = NLDB0::getOrCreateMemory({3, 3, 2, 1, 1});
  add(top, model, "used");
  model->getTerm(NLName("RDATA"))->setName(NLName("changed"));
  const auto path = std::filesystem::path(SNL_CAPNP_TEST_PATH) / "modified_db0_interface";
  SNLCapnP::dump(top->getDB(), path);
  NLUniverse::get()->destroy();
  EXPECT_THROW(SNLCapnP::load(path), NLException);
}

TEST_F(SNLCapnPPrimitiveIdentityTest, OverwriteShorterSnapshotTruncatesPayloads) {
  const auto path = std::filesystem::path(SNL_CAPNP_TEST_PATH) / "overwrite_db0_snapshot";
  const auto clean = std::filesystem::path(SNL_CAPNP_TEST_PATH) / "clean_db0_snapshot";
  auto* top = createTop();
  add(top, NLDB0::getOrCreateMemory({11, 7, 3, 2, 2}), "used");
  SNLCapnP::dump(top->getDB(), path);
  NLUniverse::get()->destroy();
  top = createTop();
  SNLCapnP::dump(top->getDB(), path);
  SNLCapnP::dump(top->getDB(), clean);
  for (const auto& file : {"snl.mf", "db0_interface.snl", "db_interface.snl", "db_implementation.snl"})
    EXPECT_TRUE(bytes(path / file) == bytes(clean / file)) << file;
}

INSTANTIATE_TEST_SUITE_P(CanonicalFamilies, SNLCapnPPrimitiveAuditTest,
    ::testing::ValuesIn(primitiveCases()), [](const auto& info) { return info.param.name; });
} // namespace
