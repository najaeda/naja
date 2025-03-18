// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLScalarNets.h"

#include "PyInterface.h"
#include "PySNLScalarNet.h"

namespace PYSNL {

using namespace naja::SNL;

PyTypeContainerObjectDefinitions(SNLScalarNets)
PyTypeContainerObjectDefinitions(SNLScalarNetsIterator)

PyContainerMethods(SNLScalarNet, SNLScalarNets)

}