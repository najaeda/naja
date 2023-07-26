# Copyright The Naja Authors.
# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0
#

import snl

def construct(db):
  primitives = snl.SNLLibrary.createPrimitives(db)
  prim = snl.SNLDesign.createPrimitive(primitives, 'PRIM')
  i = snl.SNLScalarTerm.create(prim, snl.SNLTerm.Direction.Input, 'i')
  designs = snl.SNLLibrary.create(db)
  pass

def constructDB(db):
  construct(db)