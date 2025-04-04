// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLDesigns.h"

#include "PyInterface.h"
#include "PySNLDesign.h"

namespace PYNAJA {

using namespace naja::NL;

PyTypeContainerObjectDefinitions(SNLDesigns)
PyTypeContainerObjectDefinitions(SNLDesignsIterator)

PyContainerMethods(SNLDesign, SNLDesigns)

}