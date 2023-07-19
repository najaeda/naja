// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLTerms.h"

#include "PyInterface.h"
#include "PySNLTerm.h"

namespace PYSNL {

using namespace naja::SNL;

PyTypeContainerObjectDefinitions(SNLTerms)
PyTypeContainerObjectDefinitions(SNLTermsIterator)

PyContainerMethods(SNLTerm, SNLTerms)

}
