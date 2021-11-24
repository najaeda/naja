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
  i0 = snl.SNLScalarTerm.create(model0, 'i0')
  i0Net = snl.SNLScalarNet.create(model0, 'i0')
  i0.setNet(i0Net)
  print('Term:', i0, 'in design:', i0.getDesign(), 'connected to:', i0.getNet())
  i1 = snl.SNLScalarTerm.create(model0, 'i0')
  print('Model:', model0) 

  model1 = snl.SNLDesign.create(lib1)
  d0 = snl.SNLDesign.create(lib2)
  print('Design:', d0)

  d0Input0 = snl.SNLScalarTerm.create(d0, 'Input0')
  print('Term:', d0Input0, 'in design:', d0Input0.getDesign())

  i0 = snl.SNLInstance.create(d0, model0, 'i0')
  i1 = snl.SNLInstance.create(d0, model0)
  print('Instance:', i0, ' in design:', i0.getDesign())
  print('Instance:', i1, ' in design:', i1.getDesign())


if __name__ == '__main__':
  sys.exit(main())
