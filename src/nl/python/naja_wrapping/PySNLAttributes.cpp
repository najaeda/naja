// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLAttributes.h"

#include "SNLAttributes.h"

#include "PyInterface.h"
#include "PySNLAttribute.h"

namespace PYNAJA {

using namespace naja::NL;

PyTypeContainerObjectDefinitions(SNLAttributes)
PyTypeContainerObjectDefinitions(SNLAttributesIterator)

PyContainerMethodsForNonPointers(SNLAttribute, SNLAttributes)

}