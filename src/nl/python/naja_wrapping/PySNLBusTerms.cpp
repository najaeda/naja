// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLBusTerms.h"

#include "PyInterface.h"
#include "PySNLBusTerm.h"

namespace PYNAJA {

using namespace naja::NL;

PyTypeContainerObjectDefinitions(SNLBusTerms)
PyTypeContainerObjectDefinitions(SNLBusTermsIterator)

PyContainerMethods(SNLBusTerm, SNLBusTerms)

}