// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_DESIGN_MODELING_DECORATOR_H_
#define __SNL_DESIGN_MODELING_DECORATOR_H_

namespace naja { namespace SNL {

class SNLDesign;

class SNLDesignModelingDecorator {
  public:
    static void setValue(SNLDesign* design, int value);
    static int getValue(const SNLDesign* design);
};

}} // namespace SNL // namespace naja

#endif // __SNL_DESIGN_MODELING_DECORATOR_H_