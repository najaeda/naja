// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLLibraries.h"

#include "PyInterface.h"
#include "PySNLLibrary.h"

namespace PYSNL {

using namespace naja::SNL;

PyTypeContainerObjectDefinitions(SNLLibraries)
PyTypeContainerObjectDefinitions(SNLLibrariesIterator)

PyContainerMethods(SNLLibrary, SNLLibraries)

}