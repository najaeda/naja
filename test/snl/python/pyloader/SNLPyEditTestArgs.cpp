// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLDB0.h"
#include "SNLException.h"
#include "SNLPyEdit.h"
using namespace naja::SNL;

#ifndef SNL_PYEDIT_TEST_PATH
#define SNL_PYEDIT_TEST_PATH "Undefined"
#endif

TEST(SNLPyDBEditTestArgs, test0) {
  auto scriptPath = std::filesystem::path(SNL_PYEDIT_TEST_PATH);
  scriptPath /= "naja_edit";
  scriptPath /= "naja_edit_test_args0.py";
  SNLPyEdit::najaEdit(scriptPath);
}

TEST(SNLPyDBEditTestArgs, test0WithArgs) {
  auto scriptPath = std::filesystem::path(SNL_PYEDIT_TEST_PATH);
  scriptPath /= "naja_edit";
  scriptPath /= "naja_edit_test_args0.py";
  SNLPyEdit::Args args;
  args.push_back(std::make_tuple("option1", 'b', "0"));
  args.push_back(std::make_tuple("option2", 'b', "1"));
  SNLPyEdit::najaEdit(scriptPath);
}

TEST(SNLPyDBEditTestArgs, test1) {
  auto scriptPath = std::filesystem::path(SNL_PYEDIT_TEST_PATH);
  scriptPath /= "naja_edit";
  scriptPath /= "naja_edit_test_args1.py";
  SNLPyEdit::Args args;
  args.push_back(std::make_tuple("option1", 'b', "0"));
  args.push_back(std::make_tuple("option2", 'i', "34"));
  SNLPyEdit::najaEdit(scriptPath);
}