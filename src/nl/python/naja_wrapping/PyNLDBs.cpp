// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PyNLDBs.h"

#include "PyInterface.h"
#include "PyNLDB.h"

namespace PYNAJA {

using namespace naja::NL;

PyTypeContainerObjectDefinitions(NLDBs)
PyTypeContainerObjectDefinitions(NLDBsIterator)

PyContainerMethods(NLDB, NLDBs)

}