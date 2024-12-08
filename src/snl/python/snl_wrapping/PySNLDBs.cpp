// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLDBs.h"

#include "PyInterface.h"
#include "PySNLDB.h"

namespace PYSNL {

using namespace naja::SNL;

PyTypeContainerObjectDefinitions(SNLDBs)
PyTypeContainerObjectDefinitions(SNLDBsIterator)

PyContainerMethods(SNLDB, SNLDBs)

}