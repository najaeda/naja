// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NajaProperty.h"

namespace naja {

void NajaProperty::destroy() {
    preDestroy();
    delete this;
}

void NajaProperty::preDestroy() {
}

} // namespace naja