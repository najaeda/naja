// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLNetComponents.h"

#include "PyInterface.h"
#include "PySNLNetComponent.h"

namespace PYNAJA {

using namespace naja::NL;

PyTypeContainerObjectDefinitions(SNLNetComponents)
PyTypeContainerObjectDefinitions(SNLNetComponentsIterator)

PyContainerMethods(SNLNetComponent, SNLNetComponents)

}