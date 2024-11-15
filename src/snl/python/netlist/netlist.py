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
      instTerms = []
      for term in self.equi.getInstTermOccurrences():
        instTerms.append(InstTerm(term.getPath(), term.getInstTerm()))
      return instTerms
    
    def getTopTerms(self):
      topTerms = []
      for term in self.equi.getBitTermOccurrences():
        topTerms.append(TopTerm(term.getPath(), term.getBitTerm()))
      return topTerms

class Net:
    def __init__(self, path, net):
      self.path = path
      self.net = net
    
    def getName(self):
      return self.net.getName()
    
    def getInstTerms(self):
      instTerms = []
      for term in self.net.getInstTerms():
        instTerms.append(InstTerm(self.path, term))
      return instTerms
    
    def getTopTerms(self):
      topTerms = []
      for term in self.net.getBitTerms():
        topTerms.append(TopTerm(self.path, term))
      return topTerms
      
class TopTerm:

  def __init__(self, path, term):
    self.path = path
    self.term = term
  
  def getName(self):
    return self.term.getName()

  def getDirection(self):
    return self.term.getDirection()

  def getNet(self):
    return Net(self.path, self.term.getNet())
  
  def getEuiqpotential(self):
    return Equipotential(self)
  
  def isInput(self):
    return self.term.getDirection() == snl.SNLTerm.Direction.Input
  
  def isOutput(self):
    return self.term.getDirection() == snl.SNLTerm.Direction.Output
  
class InstTerm:

  def __init__(self, path, term):
    self.path = path
    self.term = term
  
  def getName(self):
    return self.inst.getInstTerms().getName()
  
  def getNet(self):
    return Net(self.path, self.inst.getInstTerms().getNet())
  
  def getInstance(self):
    inst = self.term.getInstance()
    path = snl.SNLPath(self.path, inst)
    return Instance(path, inst)
  
  def getFlatFanout(self):
    return self.inst.getInstTerms().getFlatFanout()
  
  def getEuiqpotential(self):
    return Equipotential(self)
  
  def isInput(self):
    return self.term.getDirection() == snl.SNLTerm.Direction.Input
  
  def isOutput(self):
    return self.term.getDirection() == snl.SNLTerm.Direction.Output
  
  def getString(self):
    return str(snl.SNLInstTermOccurrence(self.path, self.term))

  
# Class that represents the instance and wrap some of the snl occurrence api
class Instance:

  def __init__(self):
    self.path =  snl.SNLPath()
    self.inst = None
  
  # Initialize the instance list of names
  def __init__(self, names):
    path =  snl.SNLPath()
    instance = None
    top = snl.SNLUniverse.get().getTopDesign()
    design = top
    for name in names:
      path = snl.SNLPath(path, top.getInstance(name))
      instance = design.getInstance(name)
      design = instance.getModel()
    self.path = path
    self.inst = instance

  # Copy Constructor
  def __init__(self, instance):
    self.inst = instance.inst
    self.path = instance.path
  
  def __init__(self, path, inst):
    self.inst = inst
    self.path = path
  
  def getChildInstance(self, name):
    instance = Instance()
    instance.inst = self.inst.getModel().getInstance(name)
    instance.path = snl.SNLPath(self.path, self.inst)
    return instance
  
  def getInstTerms(self):
    instTerms = []
    if self.inst is None:
      return instTerms
    for term in self.inst.getInstTerms():
      instTerms.append(InstTerm(self.path.getHeadPath(), term))
    return instTerms
  
  def getInstTerm(self, name):
    if self.inst is None:
      return None
    for term in self.inst.getInstTerms():
      if term.getName() == name:
        return InstTerm(self.path.getHeadPath(), term)
    return None
  
  def getTopTerms(self):
    topTerms = []
    if self.inst is None:
      top = snl.SNLUniverse.get().getTopDesign()
      for term in top.getBitTerms():
        topTerms.append(TopTerm(self.path, term))
      return topTerms
    else:
      return topTerms

  def isPrimitive(self):
    return self.inst.getModel().isPrimitive()
  
class Loader:

    def __init__(self):
      self.db_ = None
      self.primitivesLibrary_ = None

    def init(self):
      snl.SNLUniverse.create()
      self.db_ =  snl.SNLDB.create(snl.SNLUniverse.get())

    def getDB(self):
      return self.db_

    def getPrimitivesLibrary(self):
      if (self.primitivesLibrary_ is None):
          self.primitivesLibrary_ = snl.SNLLibrary.createPrimitives(self.db_)
      return self.primitivesLibrary_

    def loadVerilog(self, files):
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

def getAllPrimitivesInstances():
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



