// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PyNLLibraries.h"

#include "PyInterface.h"
#include "PyNLLibrary.h"

namespace PYNAJA {

using namespace naja::NL;

PyTypeContainerObjectDefinitions(NLLibraries)
PyTypeContainerObjectDefinitions(NLLibrariesIterator)

PyContainerMethods(NLLibrary, NLLibraries)

}
