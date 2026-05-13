// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLBundleTerms.h"

#include "PyInterface.h"
#include "PySNLBundleTerm.h"

namespace PYNAJA {

using namespace naja::NL;

PyTypeContainerObjectDefinitions(SNLBundleTerms)
PyTypeContainerObjectDefinitions(SNLBundleTermsIterator)

PyContainerMethods(SNLBundleTerm, SNLBundleTerms)

}
