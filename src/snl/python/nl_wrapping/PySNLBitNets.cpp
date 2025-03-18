// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLBitNets.h"

#include "PyInterface.h"
#include "PySNLBitNet.h"

namespace PYSNL {

using namespace naja::SNL;

PyTypeContainerObjectDefinitions(SNLBitNets)
PyTypeContainerObjectDefinitions(SNLBitNetsIterator)

PyContainerMethods(SNLBitNet, SNLBitNets)

}
