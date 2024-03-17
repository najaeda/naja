# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import snl

def construct(design):
  universe = snl.SNLUniverse.get()
  db = universe.getDB(1)
  if db is None:
    raise Exception("Cannot find the db containing the primitives and top")
  primitives = db.getLibrary("primitives")
  if primitives is None:
    raise Exception("Cannot find the primitives library")
  prim = primitives.getDesign("primitive")
  if prim is None:
    raise Exception("Cannot find the primitive design")
  snl.SNLInstance.create(design, prim, "myins")

def constructDB(db):
  construct(db)