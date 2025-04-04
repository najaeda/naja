// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLBitTerms.h"

#include "PyInterface.h"
#include "PySNLBitTerm.h"

namespace PYNAJA {

using namespace naja::NL;

PyTypeContainerObjectDefinitions(SNLBitTerms)
PyTypeContainerObjectDefinitions(SNLBitTermsIterator)

PyContainerMethods(SNLBitTerm, SNLBitTerms)

}