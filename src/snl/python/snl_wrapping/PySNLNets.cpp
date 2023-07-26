// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLNets.h"

#include "PyInterface.h"
#include "PySNLNet.h"

namespace PYSNL {

using namespace naja::SNL;

PyTypeContainerObjectDefinitions(SNLNets)
PyTypeContainerObjectDefinitions(SNLNetsIterator)

PyContainerMethods(SNLNet, SNLNets)

}