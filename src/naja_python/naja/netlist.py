# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

from naja import snl
import logging

# Class that represents the term and wrap some of the snl occurrence api
class Equipotential:
    def __init__(self, InstTerm):
      ito = snl.SNLNetComponentOccurrence(InstTerm.path, InstTerm.term)
      self.equi = snl.SNLEquipotential(ito)
    
    def getInstTerms(self):
      for term in self.equi.getInstTermOccurrences():
        yield InstTerm(term.getPath(), term.getInstTerm())
    
    def getTopTerms(self):
      for term in self.equi.getTerms():
       yield TopTerm(snl.SNLPath(), term.getBitTerm())
    
    def getAllLeafReaders(self):
      for term in self.equi.getInstTermOccurrences():
        if term.getInstTerm().getDirection() == snl.SNLTerm.Direction.Output:
          yield InstTerm(term.getPath(), term.getInstTerm())

class Net:
    def __init__(self, path, net):
      self.path = path
      self.net = net
    
    def getName(self) -> str: 
      return self.net.getName()
    
    def getInstTerms(self):
      for term in self.net.getInstTerms():
        yield InstTerm(self.path, term)
    
    def getTopTerms(self):
      for term in self.net.getBitTerms():
        yield TopTerm(self.path, term)
      
class TopTerm:

  def __init__(self, path, term):
    self.path = path
    self.term = term
  
  # Comperators first by path and then by term

  def __eq__(self, other) -> bool:
    return self.path == other.path and self.term == other.term
  
  def __ne__(self, other) -> bool:
    return not self == other
  
  def __lt__(self, other) -> bool:
    if self.path != other.path:
      return self.path < other.path
    return self.term < other.term
  
  def __le__(self, other) -> bool:
    return self < other or self == other
  
  def __gt__(self, other) -> bool:
    return not self <= other
  
  def __ge__(self, other) -> bool:
    return not self < other
  
  def getName(self) -> str:
    return self.term.getName()

  def getDirection(self) -> snl.SNLTerm.Direction:
    return self.term.getDirection()

  def getNet(self) -> Net:
    return Net(self.path, self.term.getNet())
  
  def getEquipotential(self) -> Equipotential:
    return Equipotential(self)
  
  def isInput(self) -> bool:
    return self.term.getDirection() == snl.SNLTerm.Direction.Input
  
  def isOutput(self) -> bool:
    return self.term.getDirection() == snl.SNLTerm.Direction.Output
  
class InstTerm:

  def __init__(self, path, term):
    self.path = path
    self.term = term
  
# Comparators first by path and then by term

  def __eq__(self, other) -> bool:
    return self.path == other.path and self.term == other.term
  
  def __ne__(self, other) -> bool:
    return not self == other
  
  def __lt__(self, other) -> bool:
    if self.path != other.path:
      return self.path < other.path
    return self.term < other.term
  
  def __le__(self, other) -> bool:
    return self < other or self == other
  
  def __gt__(self, other) -> bool:
    return not self <= other
  
  def __ge__(self, other) -> bool:
    return not self < other

  def getName(self) -> str:
    return self.inst.getInstTerms().getName()
  
  def getNet(self) -> Net:
    return Net(self.path, self.inst.getInstTerms().getNet())
  
  def getInstance(self):
    inst = self.term.getInstance()
    path = snl.SNLPath(self.path, inst)
    return Instance(path, inst)
  
  def getFlatFanout(self) -> Equipotential:
    return self.getEquipotential().getAllLeafReaders()
  
  def getEquipotential(self) -> Equipotential:
    return Equipotential(self)
  
  def isInput(self) -> bool:
    return self.term.getDirection() == snl.SNLTerm.Direction.Input
  
  def isOutput(self) -> bool:
    return self.term.getDirection() == snl.SNLTerm.Direction.Output
  
  def getString(self) -> str:
    return str(snl.SNLInstTermOccurrence(self.path, self.term))

def getInstanceByPath(names: list):
  path =  snl.SNLPath()
  instance = None
  top = snl.SNLUniverse.get().getTopDesign()
  design = top
  for name in names:
    path = snl.SNLPath(path, design.getInstance(name))
    instance = design.getInstance(name)
    design = instance.getModel()
  return Instance(path, instance)
  
# Class that represents the instance and wrap some of the snl occurrence api
class Instance:
  
  def __init__(self, path, inst):
    self.inst = inst
    self.path = path
  
  def __eq__(self, other) -> bool:
    return self.inst == other.inst and self.path == other.path
  
  def getChildInstance(self, name: str):
    return Instance(snl.SNLPath(self.path, self.inst.getModel().getInstance(name)), self.inst.getModel().getInstance(name))
  
  def getInstTerms(self):
    if self.inst is None:
      return
    for term in self.inst.getInstTerms():
      yield InstTerm(self.path.getHeadPath(), term)
  
  def getInstTerm(self, name: str) -> InstTerm:
    if self.inst is None:
      return None
    for term in self.inst.getInstTerms():
      if term.getName() == name:
        return InstTerm(self.path.getHeadPath(), term)
    return None
  
  def getTopTerms(self):
    if self.inst is None:
      top = snl.SNLUniverse.get().getTopDesign()
      for term in top.getBitTerms():
         yield TopTerm(self.path, term)

  def isPrimitive(self):
    return self.inst.getModel().isPrimitive()
  
  def getOutputInstTerms(self):
    for term in self.inst.getInstTerms():
      if term.getDirection() == snl.SNLTerm.Direction.Output:
        yield InstTerm(self.path.getHeadPath(), term)
  
  def deleteInstance(self, name: str):
    uniq = snl.SNLUniquifier(self.path)
    uniqPath = uniq.getPathUniqCollection()
    # Delete the last instance in uniqPath    
  
class Loader:

    def __init__(self):
      self.db_ = None
      self.primitivesLibrary_ = None

    def init(self):
      snl.SNLUniverse.create()
      self.db_ =  snl.SNLDB.create(snl.SNLUniverse.get())

    def getDB(self) -> snl.SNLDB:
      return self.db_

    def getPrimitivesLibrary(self) -> snl.SNLLibrary:
      if (self.primitivesLibrary_ is None):
          self.primitivesLibrary_ = snl.SNLLibrary.createPrimitives(self.db_)
      return self.primitivesLibrary_

    def loadVerilog(self, files: list):
      self.db_.loadVerilog(files)

    def verify(self):
      universe = snl.SNLUniverse.get()
      if universe is None:
          logging.critical('No loaded SNLUniverse')
          return 1
      top = universe.getTopDesign()
      if top is None:
          logging.critical('SNLUniverse does not contain any top SNLDesign')
          return 1
      else:
          logging.info('Found top design ' + str(top))
    
    def loadLibertyPrimitives(self, files):
      self.db_.loadLibertyPrimitives(files)

def getAllPrimitiveInstances() -> list:
  top = snl.SNLUniverse.get().getTopDesign()
  primitives = []
    
  for inst in top.getInstances():
    path = snl.SNLPath(inst)
    stack = [[inst, path]]
    while stack:
        current = stack.pop()
        currentInst = current[0]    
        currentPath = current[1]
        for instChild in currentInst.getModel().getInstances():
            pathChild = snl.SNLPath(currentPath, instChild)
            if instChild.getModel().isPrimitive():
                primitives.append(Instance(pathChild, instChild))
            stack.append([instChild, pathChild])
  return primitives