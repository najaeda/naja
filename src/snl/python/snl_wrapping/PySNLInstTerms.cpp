// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLInstTerms.h"

#include "PyInterface.h"
#include "PySNLInstTerm.h"

namespace PYSNL {

using namespace naja::SNL;

PyTypeContainerObjectDefinitions(SNLInstTerms)
PyTypeContainerObjectDefinitions(SNLInstTermsIterator)

PyContainerMethods(SNLInstTerm, SNLInstTerms)

}
