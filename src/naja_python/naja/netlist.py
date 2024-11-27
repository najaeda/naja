# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import logging
from naja import snl


class Equipotential:
    """Class that represents the term and wraps some of the snl occurrence API."""

    def __init__(self, inst_term):
        ito = snl.SNLNetComponentOccurrence(inst_term.path, inst_term.term)
        self.equi = snl.SNLEquipotential(ito)

    def get_inst_terms(self):
        for term in self.equi.getInstTermOccurrences():
            yield InstTerm(term.getPath(), term.getInstTerm())

    def get_top_terms(self):
        for term in self.equi.getTerms():
            yield TopTerm(snl.SNLPath(), term.getBitTerm())

    def get_all_leaf_readers(self):
        for term in self.equi.getInstTermOccurrences():
            if term.getInstTerm().getDirection() == snl.SNLTerm.Direction.Output:
                yield InstTerm(term.getPath(), term.getInstTerm())


class Net:
    def __init__(self, path, net):
        self.path = path
        self.net = net

    def get_name(self) -> str:
        return self.net.getName()

    def get_inst_terms(self):
        for term in self.net.getInstTerms():
            yield InstTerm(self.path, term)

    def get_top_terms(self):
        for term in self.net.getBitTerms():
            yield TopTerm(self.path, term)

    def __eq__(self, value):
        return self.net == value.net and self.path == value.path


class TopTerm:
    def __init__(self, path, term):
        self.path = path
        self.term = term

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

    def get_name(self) -> str:
        return self.term.getName()

    def get_direction(self) -> snl.SNLTerm.Direction:
        return self.term.getDirection()

    def get_net(self) -> Net:
        return Net(self.path, self.term.getNet())

    def get_equipotential(self) -> Equipotential:
        return Equipotential(self)

    def is_input(self) -> bool:
        return self.term.getDirection() == snl.SNLTerm.Direction.Input

    def is_output(self) -> bool:
        return self.term.getDirection() == snl.SNLTerm.Direction.Output


class InstTerm:
    def __init__(self, path, term):
        self.path = path
        self.term = term

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

    def get_name(self) -> str:
        return self.term.getName()

    def get_net(self) -> Net:
        return Net(self.path, self.term.getNet())

    def get_instance(self):
        inst = self.term.getInstance()
        path = snl.SNLPath(self.path, inst)
        return Instance(path, inst)

    def get_flat_fanout(self) -> Equipotential:
        return self.get_equipotential().get_all_leaf_readers()

    def get_equipotential(self) -> Equipotential:
        return Equipotential(self)

    def is_input(self) -> bool:
        return self.term.getDirection() == snl.SNLTerm.Direction.Input

    def is_output(self) -> bool:
        return self.term.getDirection() == snl.SNLTerm.Direction.Output

    def get_string(self) -> str:
        return str(snl.SNLInstTermOccurrence(self.path, self.term))

    def disconnect(self):
        term = self.term.getTerm()
        uniq = snl.SNLUniquifier(self.path)
        uniq_path = uniq.getPathUniqCollection()
        inst = tuple(uniq_path)[len(tuple(uniq_path)) - 1]
        self.term = inst.getInstTerm(term)
        self.term.setNet(snl.SNLNet())

    def connect(self, net: Net):
        term = self.term.getTerm()
        uniq = snl.SNLUniquifier(self.path)
        uniq_path = uniq.getPathUniqCollection()
        inst = tuple(uniq_path)[len(tuple(uniq_path)) - 1]
        self.term = inst.getInstTerm(term)
        self.term.setNet(net.net)


def get_instance_by_path(names: list):
    path = snl.SNLPath()
    instance = None
    top = snl.SNLUniverse.get().getTopDesign()
    design = top
    for name in names:
        path = snl.SNLPath(path, design.getInstance(name))
        instance = design.getInstance(name)
        design = instance.getModel()
    return Instance(path, instance)


class Instance:
    """Class that represents the instance and wraps some of the snl occurrence API."""

    def __init__(self, path, inst):
        self.inst = inst
        self.path = path

    def __eq__(self, other) -> bool:
        return self.inst == other.inst and self.path == other.path

    def get_child_instance(self, name: str):
        return Instance(snl.SNLPath(self.path, self.inst.getModel().getInstance(name)),
                        self.inst.getModel().getInstance(name))

    def get_child_instances(self):
        for inst in self.inst.getModel().getInstances():
            path = snl.SNLPath(self.path, inst)
            yield Instance(path, inst)

    def get_number_of_child_instances(self) -> int:
        return len(tuple(self.inst.getModel().getInstances()))

    def get_inst_terms(self):
        if self.inst is None:
            return
        for term in self.inst.getInstTerms():
            yield InstTerm(self.path.getHeadPath(), term)

    def get_inst_term(self, name: str) -> InstTerm:
        if self.inst is None:
            return None
        for term in self.inst.getInstTerms():
            if term.getBitTerm().getName() == name:
                return InstTerm(self.path.getHeadPath(), term)
        return None

    def is_primitive(self) -> bool:
        return self.inst.getModel().isPrimitive()

    def get_output_inst_terms(self):
        for term in self.inst.getInstTerms():
            if term.getDirection() == snl.SNLTerm.Direction.Output:
                yield InstTerm(self.path.getHeadPath(), term)

    def delete_instance(self, name: str):
        path = snl.SNLPath(self.path, self.inst.getModel().getInstance(name))
        uniq = snl.SNLUniquifier(path)
        uniq_path = uniq.getPathUniqCollection()
        self.inst = tuple(uniq_path)[len(tuple(uniq_path)) - 1]
        # Delete the last instance in uniq_path
        tuple(uniq_path)[len(tuple(uniq_path)) - 1].destroy()

    def get_name(self) -> str:
        return self.inst.getName()
    
    def create_child_instance(self, model, name):
        uniq = snl.SNLUniquifier(self.path)
        uniq_path = uniq.getPathUniqCollection()
        self.inst = tuple(uniq_path)[len(tuple(uniq_path)) - 1]
        design = self.inst.getModel()
        newSNLInstance = snl.SNLInstance.create(design, model, name)
        path = snl.SNLPath(self.path, newSNLInstance)
        return Instance(snl.SNLPath(self.path, newSNLInstance), newSNLInstance)

    def create_output_term(self, name: str) -> InstTerm:
        uniq = snl.SNLUniquifier(self.path)
        uniq_path = uniq.getPathUniqCollection()
        self.inst = tuple(uniq_path)[len(tuple(uniq_path)) - 1]
        design = self.inst.getModel()
        newSNLTerm = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Output, name)
        return InstTerm(self.path, newSNLTerm)
    
    def create_input_term(self, name: str) -> InstTerm:
        uniq = snl.SNLUniquifier(self.path)
        uniq_path = uniq.getPathUniqCollection()
        self.inst = tuple(uniq_path)[len(tuple(uniq_path)) - 1]
        design = self.inst.getModel()
        newSNLTerm = snl.SNLScalarTerm.create(design, snl.SNLTerm.Direction.Input, name)
        return InstTerm(self.path, newSNLTerm)
    
    def create_output_bus_term(self, name: str, width: int, offset: int) -> list:
        uniq = snl.SNLUniquifier(self.path)
        uniq_path = uniq.getPathUniqCollection()
        self.inst = tuple(uniq_path)[len(tuple(uniq_path)) - 1]
        design = self.inst.getModel()
        newSNLTerm = snl.SNLBusTerm.create(design, snl.SNLTerm.Direction.Output, width, offset, name)
        instTerms = []
        for i in range(width):
            instTerms.append(InstTerm(self.path, newSNLTerm.getBitTerm(i)))
        return instTerms
    
    def create_input_bus_term(self, name: str, width: int, offset: int) -> list:
        uniq = snl.SNLUniquifier(self.path)
        uniq_path = uniq.getPathUniqCollection()
        self.inst = tuple(uniq_path)[len(tuple(uniq_path)) - 1]
        design = self.inst.getModel()
        newSNLTerm = snl.SNLBusTerm.create(design, snl.SNLTerm.Direction.Input, width, offset, name)
        instTerms = []
        for i in range(width):
            instTerms.append(InstTerm(self.path, newSNLTerm.getBitTerm(i)))
        return instTerms

    def create_net(self, name: str) -> Net:
        uniq = snl.SNLUniquifier(self.path)
        uniq_path = uniq.getPathUniqCollection()
        self.inst = tuple(uniq_path)[len(tuple(uniq_path)) - 1]
        design = self.inst.getModel()
        newSNLNet = snl.SNLScalarNet.create(design, name)
        return Net(self.path, newSNLNet)
    
    def create_bus_net(self, name: str, width: int, offset: int) -> list:
        uniq = snl.SNLUniquifier(self.path)
        uniq_path = uniq.getPathUniqCollection()
        self.inst = tuple(uniq_path)[len(tuple(uniq_path)) - 1]
        design = self.inst.getModel()
        newSNLNet = snl.SNLBusNet.create(design, width, offset, name)
        list = []
        for i in range(width):
            list.append(Net(self.path, newSNLNet.getBitNet(i)))
        return list
    
    def get_term_list_for_bus(self, name: str) -> list:
        list = []
        bus = self.inst.getModel().getBusTerm(name)
        for i in range(bus.getWidth()):
            list.append(InstTerm(self.path.getHeadPath(), bus.getBitTerm(i)))
        return list
    
    def get_net(self, name: str) -> Net:
        for term in self.inst.getInstTerms():
            if term.getBitTerm().getName() == name:
                return Net(self.path.getHeadPath(), term.getNet())
        return None
    
    def get_net_list_for_bus(self, name: str) -> list:
        list = []
        bus = self.inst.getModel().getBusNet(name)
        for i in range(bus.getWidth()):
            list.append(Net(self.path.getHeadPath(), bus.getBitNet(i)))
        return list
    
class Top:
    def __init__(self):
        self.design = snl.SNLUniverse.get().getTopDesign()

    def get_child_instance(self, name: str) -> Instance:
        return Instance(snl.SNLPath(), self.design.getInstance(name))

    def get_child_instances(self):
        for inst in self.design.getInstances():
            path = snl.SNLPath(snl.SNLPath(), inst)
            yield Instance(path, inst)

def get_top():
    return Top()

class Loader:
    def __init__(self):
        self.db_ = None
        self.primitives_library_ = None

    def init(self):
        snl.SNLUniverse.create()
        self.db_ = snl.SNLDB.create(snl.SNLUniverse.get())

    def get_db(self) -> snl.SNLDB:
        return self.db_

    def get_primitives_library(self) -> snl.SNLLibrary:
        if self.primitives_library_ is None:
            self.primitives_library_ = snl.SNLLibrary.createPrimitives(self.db_)
        return self.primitives_library_

    def load_verilog(self, files: list):
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

    def load_liberty_primitives(self, files):
        self.db_.loadLibertyPrimitives(files)


def get_all_primitive_instances() -> list:
    top = snl.SNLUniverse.get().getTopDesign()
    primitives = []

    for inst in top.getInstances():
        path = snl.SNLPath(inst)
        stack = [[inst, path]]
        while stack:
            current = stack.pop()
            current_inst = current[0]
            current_path = current[1]
            for inst_child in current_inst.getModel().getInstances():
                path_child = snl.SNLPath(current_path, inst_child)
                if inst_child.getModel().isPrimitive():
                    primitives.append(Instance(path_child, inst_child))
                stack.append([inst_child, path_child])
    return primitives
