// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLOccurrences.h"

#include "PyInterface.h"
#include "PySNLOccurrence.h"

#include "SNLOccurrence.h"

namespace PYNAJA {

using namespace naja::NL;

PyTypeContainerObjectDefinitions(SNLOccurrences)
PyTypeContainerObjectDefinitions(SNLOccurrencesIterator)

PyContainerMethodsForNonPointers(SNLOccurrence, SNLOccurrences)

}