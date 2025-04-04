// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLInstTerms.h"

#include "PyInterface.h"
#include "PySNLInstTerm.h"

namespace PYNAJA {

using namespace naja::NL;

PyTypeContainerObjectDefinitions(SNLInstTerms)
PyTypeContainerObjectDefinitions(SNLInstTermsIterator)

PyContainerMethods(SNLInstTerm, SNLInstTerms)

}