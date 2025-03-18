// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLInstances.h"

#include "PyInterface.h"
#include "PySNLInstance.h"

namespace PYSNL {

using namespace naja::SNL;

PyTypeContainerObjectDefinitions(SNLInstances)
PyTypeContainerObjectDefinitions(SNLInstancesIterator)

PyContainerMethods(SNLInstance, SNLInstances)

}
