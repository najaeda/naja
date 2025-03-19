// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLScalarTerms.h"

#include "PyInterface.h"
#include "PySNLScalarTerm.h"

namespace PYNAJA {

using namespace naja::NL;

PyTypeContainerObjectDefinitions(SNLScalarTerms)
PyTypeContainerObjectDefinitions(SNLScalarTermsIterator)

PyContainerMethods(SNLScalarTerm, SNLScalarTerms)

}