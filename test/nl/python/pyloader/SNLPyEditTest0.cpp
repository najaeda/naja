// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>
#include <string>

#include "NLUniverse.h"
#include "NLDB0.h"
#include "NLException.h"
#include "NajaLog.h"

#include "SNLPyEdit.h"
using namespace naja::NL;

#ifndef SNL_PYEDIT_TEST_PATH
#define SNL_PYEDIT_TEST_PATH "Undefined"
#endif

#ifndef SNL_DUMP_PATH
#define SNL_DUMP_PATH "Undefined"
#endif

namespace {
std::string readFile(const std::filesystem::path& path) {
  std::ifstream stream(path);
  return std::string(
    std::istreambuf_iterator<char>(stream),
    std::istreambuf_iterator<char>());
}
}  // namespace

class SNLPyDBEditTest0: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      auto primitivesLibrary = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("primitives"));
      auto prim0 = SNLDesign::create(primitivesLibrary, SNLDesign::Type::Primitive, NLName("prim0"));
      auto designsLibrary = NLLibrary::create(db, NLName("designs"));
      auto top = SNLDesign::create(designsLibrary, NLName("top"));
      auto inst0 = SNLInstance::create(top, prim0, NLName("instance0"));
      auto bbox = SNLDesign::create(designsLibrary, NLName("bbox"));
      bbox->setType(SNLDesign::Type::UserBlackBox);
      auto inst1 = SNLInstance::create(top, bbox, NLName("instance1"));
      auto inst2 = SNLInstance::create(top, NLDB0::getAssign(), NLName("instance2"));
      universe->setTopDesign(top);
    }
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
};

TEST_F(SNLPyDBEditTest0, test) {
  auto universe = NLUniverse::get();
  ASSERT_TRUE(universe);
  auto top = universe->getTopDesign();
  ASSERT_TRUE(top);
  auto instance0 = top->getInstance(NLName("instance0"));
  ASSERT_TRUE(instance0);
  EXPECT_EQ(NLName("instance0"), instance0->getName());
  auto instance1 = top->getInstance(NLName("instance1"));
  ASSERT_TRUE(instance1);
  EXPECT_EQ(NLName("instance1"), instance1->getName());
  auto instance2 = top->getInstance(NLName("instance2"));
  ASSERT_TRUE(instance2);
  EXPECT_EQ(NLName("instance2"), instance2->getName());
  auto scriptPath = std::filesystem::path(SNL_PYEDIT_TEST_PATH);
  scriptPath /= "edit";
  scriptPath /= "edit_test0.py";

  const auto nativeLogPath =
    std::filesystem::path(SNL_DUMP_PATH) / "embedded_python_native.log";
  const auto pythonLogPath =
    std::filesystem::current_path() / "edit_test0.log";
  std::filesystem::remove(nativeLogPath);
  std::filesystem::remove(pythonLogPath);
  naja::log::clearSinks();
  naja::log::addFileSink(nativeLogPath.string());
  naja::log::setLevel(spdlog::level::info);
  SNLPyEdit::edit(scriptPath);
  naja::log::get()->flush();
  const auto nativeLog = readFile(nativeLogPath);
  EXPECT_NE(
    std::string::npos,
    nativeLog.find("[naja] [info] Found top design"));
  naja::log::clearSinks();

  ASSERT_TRUE(std::filesystem::exists(pythonLogPath));
  EXPECT_NE(
    std::string::npos,
    readFile(pythonLogPath).find("INFO:root:Found top design"));
  EXPECT_EQ(NLName("instance00"), instance0->getName());
  EXPECT_EQ(nullptr, top->getInstance(NLName("instance0")));
  EXPECT_EQ(instance0, top->getInstance(NLName("instance00")));

  EXPECT_EQ(NLName("bbox_instance"), instance1->getName());
  EXPECT_EQ(nullptr, top->getInstance(NLName("instance1")));
  EXPECT_EQ(instance1, top->getInstance(NLName("bbox_instance")));

  EXPECT_EQ(NLName("assign_instance"), instance2->getName());
  EXPECT_EQ(nullptr, top->getInstance(NLName("instance2")));
  EXPECT_EQ(instance2, top->getInstance(NLName("assign_instance")));
}

TEST_F(SNLPyDBEditTest0, testEditDBError) {
  auto db = NLDB::create(NLUniverse::get());
  auto scriptPath = std::filesystem::path(SNL_PYEDIT_TEST_PATH);
  scriptPath /= "edit";
  scriptPath /= "edit_faulty.py";
  try {
    SNLPyEdit::edit(scriptPath);
    FAIL() << "Expected NLException";
  } catch (const NLException& e) {
    std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Error while calling edit"));
    EXPECT_NE(std::string::npos, reason.find("NameError"));
  }
}
