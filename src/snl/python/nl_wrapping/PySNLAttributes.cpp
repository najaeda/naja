// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLAttributes.h"

#include "PyInterface.h"
#include "PySNLAttribute.h"

namespace PYSNL {

using namespace naja::SNL;

PyTypeContainerObjectDefinitions(SNLAttributes)
PyTypeContainerObjectDefinitions(SNLAttributesIterator)

PyContainerMethodsForNonPointers(SNLAttribute, SNLAttributes)

}