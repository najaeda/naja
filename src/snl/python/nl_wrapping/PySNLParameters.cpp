// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLParameters.h"

#include "PyInterface.h"
#include "PySNLParameter.h"

namespace PYSNL {

using namespace naja::SNL;

PyTypeContainerObjectDefinitions(SNLParameters)
PyTypeContainerObjectDefinitions(SNLParametersIterator)

PyContainerMethods(SNLParameter, SNLParameters)

}
