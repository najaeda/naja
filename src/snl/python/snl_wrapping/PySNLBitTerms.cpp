// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLBitTerms.h"

#include "PyInterface.h"
#include "PySNLBitTerm.h"

namespace PYSNL {

using namespace naja::SNL;

PyTypeContainerObjectDefinitions(SNLBitTerms)
PyTypeContainerObjectDefinitions(SNLBitTermsIterator)

PyContainerMethods(SNLBitTerm, SNLBitTerms)

}