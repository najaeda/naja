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
  print('Model:', model0)
  i0 = snl.SNLScalarTerm.create(model0, snl.SNLTerm.Direction.Input, 'i0')
  i0Net = snl.SNLScalarNet.create(model0, 'i0')
  i0.setNet(i0Net)
  print('ScalarTerm:', i0, 'in design:', i0.getDesign(), 'connected to:', i0.getNet())
  i1 = snl.SNLBusTerm.create(model0, snl.SNLTerm.Direction.Input, 31, 0, 'i1')
  print('BusTerm:', i1, 'in design:', i0.getDesign(), 'connected to:', i1.getNet())

  model1 = snl.SNLDesign.create(lib1)
  print('Model:', model1)
  d0 = snl.SNLDesign.create(lib2)
  print('Design:', d0)

  d0Input0 = snl.SNLScalarTerm.create(d0, snl.SNLTerm.Direction.Input, 'Input0')
  print('ScalarTerm:', d0Input0, 'in design:', d0Input0.getDesign())

  i0 = snl.SNLInstance.create(d0, model0, 'i0')
  i1 = snl.SNLInstance.create(d0, model0)
  print('Instance:', i0, ' in design:', i0.getDesign())
  print('Instance:', i1, ' in design:', i1.getDesign())

  return 0

if __name__ == '__main__':
  sys.exit(main())
