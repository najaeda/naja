# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import snl

def construct(design):
  #find primitives library
  universe = snl.SNLUniverse.get()
  db = universe.getDB(1)
  if db is None:
    raise Exception("Cannot find the db containing the primitives and top")
  primitives = db.getLibrary("primitives")
  if primitives is None:
    raise Exception("Cannot find the primitives library")
  square = primitives.getDesign("square")
  if square is None:
    raise Exception("Cannot find the square primitive")
  n = square.getScalarTerm("n")
  if n is None:
    raise Exception("Cannot find the n term in the square primitive")
  s = square.getScalarTerm("s")
  if s is None:
    raise Exception("Cannot find the s term in the square primitive")
  w = square.getScalarTerm("w")
  if w is None:
    raise Exception("Cannot find the w term in the square primitive")
  e = square.getScalarTerm("e")
  if e is None:
    raise Exception("Cannot find the e term in the square primitive")

  #create a huge matrix
  for i in range(0, 1000):
    for j in range(0, 1000):
      instance = snl.SNLInstance.create(design, square, "s_" + str(i) + "_" + str(j))
      ni = instance.getInstTerm(n)
      si = instance.getInstTerm(s)
      wi = instance.getInstTerm(w)
      ei = instance.getInstTerm(e)
      #create net for south
      southNet = snl.SNLScalarNet.create(design, "n_" + str(i) + "_" + str(j))
      si.setNet(southNet)
      if i != 0:
        upperInstance = design.getInstance("s_" + str(i - 1) + "_" + str(j))
        if upperInstance is None:
          raise Exception("Cannot find the upper instance")
        upperInstanceS = upperInstance.getInstTerm(s)
        upperNet = upperInstanceS.getNet()
        if upperNet is None:
          raise Exception("Cannot find the upper net")
        ni.setNet(upperNet)



def constructDesign(design):
  construct(design)