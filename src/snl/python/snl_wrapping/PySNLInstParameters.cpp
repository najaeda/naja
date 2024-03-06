// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLInstParameters.h"

#include "PyInterface.h"
#include "PySNLInstParameter.h"

namespace PYSNL {

using namespace naja::SNL;

PyTypeContainerObjectDefinitions(SNLInstParameters)
PyTypeContainerObjectDefinitions(SNLInstParametersIterator)

PyContainerMethods(SNLInstParameter, SNLInstParameters)

}
