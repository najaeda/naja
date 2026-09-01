
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <cstddef>
#include <string>
#include <vector>

#include "NajaPrivateProperty.h"
#include "NLUniverse.h"

#include "SNLAttributes.h"
#include "SNLInstance.h"
#include "SNLInstParameter.h"
#include "SNLParameter.h"
#include "SNLScalarTerm.h"
#include "SNLTruthTable.h"
#include "SNLDesignModeling.h"
using namespace naja::NL;

namespace {

class WrongTruthTableProperty: public naja::NajaPrivateProperty {
  public:
    static WrongTruthTableProperty* create(naja::NajaObject* object) {
      preCreate(object, Name);
      auto property = new WrongTruthTableProperty();
      property->postCreate(object);
      return property;
    }

    std::string getName() const override { return Name; }
    std::string getString() const override { return Name; }

  private:
    static const inline std::string Name = "SNLDesignTruthTableProperty";
};

struct ParameterTruthTableDesign {
  SNLDesign* design {nullptr};
  std::vector<SNLScalarTerm*> inputs {};
  SNLScalarTerm* output {nullptr};
  SNLParameter* parameter {nullptr};

  SNLDesignModeling::BitTerms getInputs() const {
    return SNLDesignModeling::BitTerms(inputs.begin(), inputs.end());
  }
};

ParameterTruthTableDesign createParameterTruthTableDesign(
    NLLibrary* primitives,
    const std::string& name,
    size_t inputCount,
    const std::string& parameterValue) {
  ParameterTruthTableDesign result;
  result.design = SNLDesign::create(
      primitives, SNLDesign::Type::Primitive, NLName(name));
  for (size_t index = 0; index < inputCount; ++index) {
    result.inputs.push_back(SNLScalarTerm::create(
        result.design, SNLTerm::Direction::Input,
        NLName("I" + std::to_string(index))));
  }
  result.output = SNLScalarTerm::create(
      result.design, SNLTerm::Direction::Output, NLName("O"));
  result.parameter = SNLParameter::create(
      result.design, NLName("INIT"), SNLParameter::Type::Binary,
      parameterValue);
  return result;
}

}  // namespace

class SNLDesignTruthTableTest0: public ::testing::Test {
  protected:
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
};

TEST_F(SNLDesignTruthTableTest0, testTruthTablesConflictError) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto design = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("design"));
  auto i0 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("I0"));
  auto i1 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("I1"));
  auto o = SNLScalarTerm::create(design, SNLTerm::Direction::Output, NLName("O"));
  //set truth table
  SNLDesignModeling::setTruthTable(design, SNLTruthTable(2, 0x5, SNLTruthTable::fullDependencies(2)));
  EXPECT_THROW(SNLDesignModeling::setTruthTable(design, SNLTruthTable(2, 0x1, SNLTruthTable::fullDependencies(2))), NLException);
}

TEST_F(SNLDesignTruthTableTest0, testDesignAttributeWithoutTruthTable) {
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto design = SNLDesign::create(
      prims, SNLDesign::Type::Primitive, NLName("attributed"));
  SNLScalarTerm::create(
      design, SNLTerm::Direction::Input, NLName("I"));
  auto output = SNLScalarTerm::create(
      design, SNLTerm::Direction::Output, NLName("O"));
  const SNLAttribute attribute(
      NLName("blackbox"),
      SNLAttributeValue(SNLAttributeValue::Type::NUMBER, "1"));
  SNLAttributes::addAttribute(design, attribute);

  EXPECT_EQ(0, SNLDesignModeling::getTruthTableCount(design));
  EXPECT_FALSE(SNLDesignModeling::getTruthTable(design).isInitialized());
  EXPECT_FALSE(
      SNLDesignModeling::getTruthTable(design, output->getOrderID())
          .isInitialized());

  const std::vector<SNLAttribute> attributes(
      SNLAttributes::getAttributes(design).begin(),
      SNLAttributes::getAttributes(design).end());
  ASSERT_EQ(1, attributes.size());
  EXPECT_EQ(attribute, attributes[0]);
}

TEST_F(SNLDesignTruthTableTest0, testDesignAttributeAndTruthTableCoexist) {
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto design = SNLDesign::create(
      prims, SNLDesign::Type::Primitive, NLName("attributed_with_tt"));
  SNLScalarTerm::create(
      design, SNLTerm::Direction::Input, NLName("I"));
  auto output = SNLScalarTerm::create(
      design, SNLTerm::Direction::Output, NLName("O"));
  const SNLAttribute attribute(
      NLName("blackbox"),
      SNLAttributeValue(SNLAttributeValue::Type::NUMBER, "1"));
  SNLAttributes::addAttribute(design, attribute);
  const SNLTruthTable truthTable(
      1, 0b01, SNLTruthTable::fullDependencies(1));
  SNLDesignModeling::setTruthTable(design, truthTable);

  EXPECT_EQ(1, SNLDesignModeling::getTruthTableCount(design));
  EXPECT_EQ(truthTable, SNLDesignModeling::getTruthTable(design));
  EXPECT_EQ(
      truthTable,
      SNLDesignModeling::getTruthTable(design, output->getOrderID()));

  const std::vector<SNLAttribute> attributes(
      SNLAttributes::getAttributes(design).begin(),
      SNLAttributes::getAttributes(design).end());
  ASSERT_EQ(1, attributes.size());
  EXPECT_EQ(attribute, attributes[0]);
}

TEST_F(SNLDesignTruthTableTest0, testWrongTruthTablePropertyTypeIsIgnored) {
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto design = SNLDesign::create(
      prims, SNLDesign::Type::Primitive, NLName("wrong_tt_property"));
  auto output = SNLScalarTerm::create(
      design, SNLTerm::Direction::Output, NLName("O"));
  WrongTruthTableProperty::create(design);

  EXPECT_EQ(0, SNLDesignModeling::getTruthTableCount(design));
  EXPECT_FALSE(SNLDesignModeling::getTruthTable(design).isInitialized());
  EXPECT_FALSE(
      SNLDesignModeling::getTruthTable(design, output->getOrderID())
          .isInitialized());
}

TEST_F(SNLDesignTruthTableTest0, testTruthTableFromInstanceParameter) {
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto library = NLLibrary::create(db, NLName("designs"));
  auto lut = SNLDesign::create(
      prims, SNLDesign::Type::Primitive, NLName("LUT2"));
  auto i0 = SNLScalarTerm::create(
      lut, SNLTerm::Direction::Input, NLName("I0"));
  auto i1 = SNLScalarTerm::create(
      lut, SNLTerm::Direction::Input, NLName("I1"));
  auto output = SNLScalarTerm::create(
      lut, SNLTerm::Direction::Output, NLName("O"));
  auto init = SNLParameter::create(
      lut, NLName("INIT"), SNLParameter::Type::Binary, "4'h0");
  SNLDesignModeling::setTruthTableFromParameter(
      lut, output, {i0, i1}, init);

  auto top = SNLDesign::create(library, NLName("top"));
  auto xorInstance = SNLInstance::create(top, lut, NLName("xor_lut"));
  auto andInstance = SNLInstance::create(top, lut, NLName("and_lut"));
  auto defaultInstance = SNLInstance::create(top, lut, NLName("default_lut"));
  auto xorInit = SNLInstParameter::create(xorInstance, init, "4'h6");
  SNLInstParameter::create(andInstance, init, "4'b1000");

  EXPECT_EQ(1, SNLDesignModeling::getTruthTableCount(lut));
  EXPECT_TRUE(SNLDesignModeling::hasTruthTableFromParameter(
      lut, output->getOrderID()));
  EXPECT_EQ(
      0,
      static_cast<uint64_t>(SNLDesignModeling::getTruthTable(lut).bits()));
  EXPECT_EQ(
      0x6,
      static_cast<uint64_t>(
          SNLDesignModeling::getTruthTable(xorInstance).bits()));
  EXPECT_EQ(
      0x8,
      static_cast<uint64_t>(SNLDesignModeling::getTruthTable(
          andInstance, output->getOrderID()).bits()));
  EXPECT_EQ(
      0,
      static_cast<uint64_t>(
          SNLDesignModeling::getTruthTable(defaultInstance).bits()));

  xorInit->setValue("4'h9");
  EXPECT_EQ(
      0x9,
      static_cast<uint64_t>(
          SNLDesignModeling::getTruthTable(xorInstance).bits()));
  xorInit->setValue("4'hx");
  EXPECT_THROW(
      SNLDesignModeling::getTruthTable(xorInstance), NLException);
}

TEST_F(SNLDesignTruthTableTest0,
       testTruthTableParameterLiteralFormsAndCaching) {
  NLUniverse::create();
  auto* db = NLDB::create(NLUniverse::get());
  auto* primitives = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto* library = NLLibrary::create(db, NLName("designs"));

  auto decimalLut = createParameterTruthTableDesign(
      primitives, "decimal_lut", 2, "4'd6");
  SNLDesignModeling::setTruthTableFromParameter(
      decimalLut.design, decimalLut.output, decimalLut.getInputs(),
      decimalLut.parameter);
  EXPECT_EQ(
      0x6,
      static_cast<uint64_t>(SNLDesignModeling::getTruthTable(
          decimalLut.design, decimalLut.output->getOrderID()).bits()));

  auto* top = SNLDesign::create(library, NLName("top"));
  auto* instance = SNLInstance::create(
      top, decimalLut.design, NLName("hex_instance"));
  SNLInstParameter::create(instance, decimalLut.parameter, "0x9");
  const auto instanceTable = SNLDesignModeling::getTruthTable(instance);
  EXPECT_EQ(0x9, static_cast<uint64_t>(instanceTable.bits()));
  EXPECT_EQ(instanceTable, SNLDesignModeling::getTruthTable(instance));

  constexpr uint64_t SixInputBits = 0x6996966996696996ULL;
  auto sixInputLut = createParameterTruthTableDesign(
      primitives, "six_input_lut", 6, "64'h6996966996696996");
  SNLDesignModeling::setTruthTableFromParameter(
      sixInputLut.design, sixInputLut.output, sixInputLut.getInputs(),
      sixInputLut.parameter);
  EXPECT_EQ(
      SixInputBits,
      static_cast<uint64_t>(
          SNLDesignModeling::getTruthTable(sixInputLut.design).bits()));
}

TEST_F(SNLDesignTruthTableTest0, testTruthTableParameterLiteralEdgeCases) {
  NLUniverse::create();
  auto* db = NLDB::create(NLUniverse::get());
  auto* primitives = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto* library = NLLibrary::create(db, NLName("designs"));
  auto lut = createParameterTruthTableDesign(
      primitives, "literal_edge_cases", 2, "4'h0");
  SNLDesignModeling::setTruthTableFromParameter(
      lut.design, lut.output, lut.getInputs(), lut.parameter);
  auto* top = SNLDesign::create(library, NLName("top"));

  size_t instanceIndex = 0;
  auto getInstanceTable = [&](const std::string& value) {
    auto* instance = SNLInstance::create(
        top, lut.design,
        NLName("literal_" + std::to_string(instanceIndex++)));
    SNLInstParameter::create(instance, lut.parameter, value);
    return SNLDesignModeling::getTruthTable(instance);
  };

  EXPECT_EQ(
      0xA,
      static_cast<uint64_t>(getInstanceTable("4' h_A").bits()));
  EXPECT_EQ(
      0xA,
      static_cast<uint64_t>(getInstanceTable("4'So12").bits()));
  EXPECT_EQ(
      0xF,
      static_cast<uint64_t>(getInstanceTable("'1").bits()));
  EXPECT_EQ(
      0,
      static_cast<uint64_t>(getInstanceTable("'0").bits()));

  EXPECT_THROW(getInstanceTable("4'h"), NLException);
  EXPECT_THROW(getInstanceTable("4'hg"), NLException);
  EXPECT_THROW(getInstanceTable("4'b2"), NLException);
  EXPECT_THROW(
      getInstanceTable("18446744073709551616"),
      NLException);
  EXPECT_THROW(getInstanceTable(" _ \t\n"), NLException);
  EXPECT_THROW(getInstanceTable("4'"), NLException);
  EXPECT_THROW(getInstanceTable("'x"), NLException);
}

TEST_F(SNLDesignTruthTableTest0,
       testTruthTableParameterValidationErrors) {
  NLUniverse::create();
  auto* db = NLDB::create(NLUniverse::get());
  auto* primitives = NLLibrary::create(db, NLLibrary::Type::Primitives);

  auto emptyInputs = createParameterTruthTableDesign(
      primitives, "empty_inputs", 1, "2'b01");
  EXPECT_THROW(
      SNLDesignModeling::setTruthTableFromParameter(
          emptyInputs.design, emptyInputs.output, {}, emptyInputs.parameter),
      NLException);

  auto tooManyInputs = createParameterTruthTableDesign(
      primitives, "too_many_inputs", 7, "128'h0");
  EXPECT_THROW(
      SNLDesignModeling::setTruthTableFromParameter(
          tooManyInputs.design, tooManyInputs.output,
          tooManyInputs.getInputs(), tooManyInputs.parameter),
      NLException);

  auto conventional = createParameterTruthTableDesign(
      primitives, "conventional", 2, "4'h6");
  SNLDesignModeling::setTruthTable(
      conventional.design,
      SNLTruthTable(2, 0x6, SNLTruthTable::fullDependencies(2)));
  EXPECT_THROW(
      SNLDesignModeling::setTruthTableFromParameter(
          conventional.design, conventional.output, conventional.getInputs(),
          conventional.parameter),
      NLException);

  auto invalidInput = createParameterTruthTableDesign(
      primitives, "invalid_input", 2, "4'h6");
  auto foreignInput = createParameterTruthTableDesign(
      primitives, "foreign_input", 1, "2'b01");
  EXPECT_THROW(
      SNLDesignModeling::setTruthTableFromParameter(
          invalidInput.design, invalidInput.output,
          {invalidInput.inputs[0], foreignInput.inputs[0]},
          invalidInput.parameter),
      NLException);

  auto outOfOrder = createParameterTruthTableDesign(
      primitives, "out_of_order", 2, "4'h6");
  EXPECT_THROW(
      SNLDesignModeling::setTruthTableFromParameter(
          outOfOrder.design, outOfOrder.output,
          {outOfOrder.inputs[1], outOfOrder.inputs[0]}, outOfOrder.parameter),
      NLException);

  auto parameterizedArcs = createParameterTruthTableDesign(
      primitives, "parameterized_arcs", 2, "4'h6");
  SNLDesignModeling::setParameter(
      parameterizedArcs.design, "INIT", parameterizedArcs.parameter->getValue());
  EXPECT_THROW(
      SNLDesignModeling::setTruthTableFromParameter(
          parameterizedArcs.design, parameterizedArcs.output,
          parameterizedArcs.getInputs(), parameterizedArcs.parameter),
      NLException);

  auto overflowingOffset = createParameterTruthTableDesign(
      primitives, "overflowing_offset", 2, "64'h0");
  EXPECT_THROW(
      SNLDesignModeling::setTruthTableFromParameter(
          overflowingOffset.design, overflowingOffset.output,
          overflowingOffset.getInputs(), overflowingOffset.parameter, 61),
      NLException);

  auto invalidRadix = createParameterTruthTableDesign(
      primitives, "invalid_radix", 2, "4'q6");
  EXPECT_THROW(
      SNLDesignModeling::setTruthTableFromParameter(
          invalidRadix.design, invalidRadix.output, invalidRadix.getInputs(),
          invalidRadix.parameter),
      NLException);
}

TEST_F(SNLDesignTruthTableTest0,
       testTruthTableParameterArgumentValidation) {
  NLUniverse::create();
  auto* db = NLDB::create(NLUniverse::get());
  auto* primitives = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto* library = NLLibrary::create(db, NLName("designs"));
  auto target = createParameterTruthTableDesign(
      primitives, "argument_target", 2, "4'h6");
  auto foreign = createParameterTruthTableDesign(
      primitives, "argument_foreign", 1, "2'b01");

  EXPECT_THROW(
      SNLDesignModeling::setTruthTableFromParameter(
          nullptr, target.output, target.getInputs(), target.parameter),
      NLException);
  EXPECT_THROW(
      SNLDesignModeling::setTruthTableFromParameter(
          target.design, nullptr, target.getInputs(), target.parameter),
      NLException);
  EXPECT_THROW(
      SNLDesignModeling::setTruthTableFromParameter(
          target.design, target.output, target.getInputs(), nullptr),
      NLException);

  auto* nonPrimitive = SNLDesign::create(library, NLName("non_primitive"));
  auto* nonPrimitiveInput = SNLScalarTerm::create(
      nonPrimitive, SNLTerm::Direction::Input, NLName("I"));
  auto* nonPrimitiveOutput = SNLScalarTerm::create(
      nonPrimitive, SNLTerm::Direction::Output, NLName("O"));
  auto* nonPrimitiveParameter = SNLParameter::create(
      nonPrimitive, NLName("INIT"), SNLParameter::Type::Binary, "2'b01");
  EXPECT_THROW(
      SNLDesignModeling::setTruthTableFromParameter(
          nonPrimitive, nonPrimitiveOutput, {nonPrimitiveInput},
          nonPrimitiveParameter),
      NLException);

  EXPECT_THROW(
      SNLDesignModeling::setTruthTableFromParameter(
          target.design, target.inputs[0], target.getInputs(),
          target.parameter),
      NLException);
  EXPECT_THROW(
      SNLDesignModeling::setTruthTableFromParameter(
          target.design, foreign.output, target.getInputs(), target.parameter),
      NLException);
  EXPECT_THROW(
      SNLDesignModeling::setTruthTableFromParameter(
          target.design, target.output, target.getInputs(), foreign.parameter),
      NLException);
  EXPECT_THROW(
      SNLDesignModeling::setTruthTableFromParameter(
          target.design, target.output, {target.inputs[0], nullptr},
          target.parameter),
      NLException);
}

TEST_F(SNLDesignTruthTableTest0,
       testTruthTableParameterConflictsAndMultipleOutputs) {
  NLUniverse::create();
  auto* db = NLDB::create(NLUniverse::get());
  auto* primitives = NLLibrary::create(db, NLLibrary::Type::Primitives);

  auto lut = createParameterTruthTableDesign(
      primitives, "conflicting_lut", 2, "4'h6");
  SNLDesignModeling::setTruthTableFromParameter(
      lut.design, lut.output, lut.getInputs(), lut.parameter);
  EXPECT_THROW(
      SNLDesignModeling::setTruthTableFromParameter(
          lut.design, lut.output, lut.getInputs(), lut.parameter),
      NLException);
  const SNLTruthTable table(
      2, 0x6, SNLTruthTable::fullDependencies(2));
  EXPECT_THROW(SNLDesignModeling::setTruthTable(lut.design, table), NLException);
  EXPECT_THROW(
      SNLDesignModeling::setTruthTables(lut.design, {table}), NLException);

  auto* multiOutput = SNLDesign::create(
      primitives, SNLDesign::Type::Primitive, NLName("multi_output_lut"));
  auto* i0 = SNLScalarTerm::create(
      multiOutput, SNLTerm::Direction::Input, NLName("I0"));
  auto* i1 = SNLScalarTerm::create(
      multiOutput, SNLTerm::Direction::Input, NLName("I1"));
  auto* o0 = SNLScalarTerm::create(
      multiOutput, SNLTerm::Direction::Output, NLName("O0"));
  auto* o1 = SNLScalarTerm::create(
      multiOutput, SNLTerm::Direction::Output, NLName("O1"));
  auto* init = SNLParameter::create(
      multiOutput, NLName("INIT"), SNLParameter::Type::Binary, "8'h96");
  SNLDesignModeling::setTruthTableFromParameter(
      multiOutput, o0, {i0, i1}, init, 0);
  SNLDesignModeling::setTruthTableFromParameter(
      multiOutput, o1, {i0, i1}, init, 4);

  EXPECT_EQ(2, SNLDesignModeling::getTruthTableCount(multiOutput));
  EXPECT_THROW(SNLDesignModeling::getTruthTable(multiOutput), NLException);
  EXPECT_EQ(
      0x6,
      static_cast<uint64_t>(
          SNLDesignModeling::getTruthTable(multiOutput, o0->getOrderID())
              .bits()));
  EXPECT_EQ(
      0x9,
      static_cast<uint64_t>(
          SNLDesignModeling::getTruthTable(multiOutput, o1->getOrderID())
              .bits()));
}

TEST_F(SNLDesignTruthTableTest0, testTruthTableParameterAPIFallbacks) {
  NLUniverse::create();
  auto* db = NLDB::create(NLUniverse::get());
  auto* primitives = NLLibrary::create(db, NLLibrary::Type::Primitives);

  auto unmodeled = createParameterTruthTableDesign(
      primitives, "unmodeled", 1, "2'b01");
  EXPECT_FALSE(SNLDesignModeling::hasTruthTableFromParameter(
      unmodeled.design, unmodeled.output->getOrderID()));
  EXPECT_THROW(
      SNLDesignModeling::getTruthTable(
      static_cast<const SNLInstance*>(nullptr), 0),
      NLException);
  EXPECT_NO_THROW(SNLDesignModeling::invalidateTruthTableCache(nullptr));
}
