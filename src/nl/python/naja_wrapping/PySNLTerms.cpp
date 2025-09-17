// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLTerms.h"

#include "PyInterface.h"
#include "PySNLTerm.h"

namespace PYNAJA {

using namespace naja::NL;

PyTypeContainerObjectDefinitions(SNLTerms)
PyTypeContainerObjectDefinitions(SNLTermsIterator)

PyContainerMethods(SNLTerm, SNLTerms)

}