// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PyNLClocks.h"

#include "PyInterface.h"
#include "PyNLClock.h"

namespace PYNAJA {

using namespace naja::NL;

PyTypeContainerObjectDefinitions(NLClocks)
PyTypeContainerObjectDefinitions(NLClocksIterator)

PyContainerMethods(NLClock, NLClocks)

}
