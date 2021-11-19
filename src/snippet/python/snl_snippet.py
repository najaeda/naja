import sys
import snl

def main() -> int:
  u = snl.SNLUniverse.create()
  db1 = snl.SNLDB.create(u)
  db2 = snl.SNLDB.create(u)
  lib0 = snl.SNLLibrary.create(db1)
  lib1 = snl.SNLLibrary.create(db2)
  lib2 = snl.SNLLibrary.create(lib0)

  model0 = snl.SNLDesign.create(lib1)
  model1 = snl.SNLDesign.create(lib1)
  d0 = snl.SNLDesign.create(lib2)
  print('Design:', d0)

  i0 = snl.SNLScalarTerm.create(d0, "I0")
  print(i0)

  i0 = snl.SNLInstance.create(d0, model0, "i0")
  i1 = snl.SNLInstance.create(d0, model0)
  print('Instance:', i0)
  print('Instance:', i1)


if __name__ == '__main__':
  sys.exit(main())  # next section explains the use of sys.exit
