// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLParameters.h"

#include "PyInterface.h"
#include "PySNLParameter.h"

namespace PYNAJA {

using namespace naja::NL;

PyTypeContainerObjectDefinitions(SNLParameters)
PyTypeContainerObjectDefinitions(SNLParametersIterator)

PyContainerMethods(SNLParameter, SNLParameters)

}