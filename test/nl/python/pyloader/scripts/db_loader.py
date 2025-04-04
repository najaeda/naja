# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import naja

def construct(db):
  primitives = naja.NLLibrary.createPrimitives(db)
  prim = naja.SNLDesign.createPrimitive(primitives, 'PRIM')
  i = naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Input, 'i')
  designs = naja.NLLibrary.create(db)

def constructDB(db):
  construct(db)
