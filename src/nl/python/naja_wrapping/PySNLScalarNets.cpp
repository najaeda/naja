// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLScalarNets.h"

#include "PyInterface.h"
#include "PySNLScalarNet.h"

namespace PYNAJA {

using namespace naja::NL;

PyTypeContainerObjectDefinitions(SNLScalarNets)
PyTypeContainerObjectDefinitions(SNLScalarNetsIterator)

PyContainerMethods(SNLScalarNet, SNLScalarNets)

}