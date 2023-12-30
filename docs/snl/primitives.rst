Primitives Management
---------------------

New primitives library can be added. The primitives library is a python script.

.. code-block:: python
    import snl

    # define a combinatorial AND2 Primitive with:
    # - 2 scalar inputs: I0 and I1
    # - 1 scalar output: O
    def constructAND2(lib):
      and2 = snl.SNLDesign.createPrimitive(lib, "AND2")
      i0 = snl.SNLScalarTerm.create(and2, snl.SNLTerm.Direction.Input, "I0")
      i1 = snl.SNLScalarTerm.create(and2, snl.SNLTerm.Direction.Input, "I1")
      o = snl.SNLScalarTerm.create(and2, snl.SNLTerm.Direction.Output, "O")
      snl.SNLDesign.addCombinatorialArcs([i0, i1], o)

    # define a sequential FD Primitive with:
    # - 1 scalar input: D, 1 scalar clock input: C
    # - 1 scalar output: Q
    # - 1 a binary parameter "MASK" of size 1 with default value "0b0" 
    def constructFD(lib):
      fd = snl.SNLDesign.createPrimitive(lib, "FD")
      q = snl.SNLScalarTerm.create(fd, snl.SNLTerm.Direction.Output, "Q")
      c = snl.SNLScalarTerm.create(fd, snl.SNLTerm.Direction.Input, "C")
      d = snl.SNLScalarTerm.create(fd, snl.SNLTerm.Direction.Input, "D")
      snl.SNLParameter.create_binary(fd, "MASK", 1, 0b0)
      snl.SNLDesign.addInputsToClockArcs(d, c)
      snl.SNLDesign.addClockToOutputsArcs(c, q)

    #The "primitives" script needs to define a constructPrimitives function
    # taking a primitives library to construct.
    def constructPrimitives(lib):
      constructAND2(lib)
      constructREG(lib)