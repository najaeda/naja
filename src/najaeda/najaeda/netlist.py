# SPDX-FileCopyrightText: 2024 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

from najaeda import snl


class Equipotential:
    """Class that represents the term and wraps
       some of the snl occurrence API.
    """

    def __init__(self, inst_term):
        ito = snl.SNLNetComponentOccurrence(
            inst_term.path.getHeadPath(),
            inst_term.term
        )
        self.equi = snl.SNLEquipotential(ito)

    def __eq__(self, value):
        return self.equi == value.equi

    def get_inst_terms(self):
        for term in self.equi.getInstTermOccurrences():
            yield InstTerm(
                snl.SNLPath(term.getPath(), term.getInstTerm().getInstance()),
                term.getInstTerm()
            )

    def get_top_terms(self):
        for term in self.equi.getTerms():
            yield TopTerm(snl.SNLPath(), term.getBitTerm())

    def get_all_leaf_readers(self):
        for term in self.equi.getInstTermOccurrences():
            direction = term.getInstTerm().getDirection()
            if direction == snl.SNLTerm.Direction.Output:
                yield InstTerm(
                    snl.SNLPath(
                        term.getPath(),
                        term.getInstTerm().getInstance()
                    ),
                    term.getInstTerm()
                )


class Net:
    def __init__(self, path, net):
        self.path = path
        self.net = net

    def __eq__(self, value):
        return self.net == value.net and self.path == value.path

    def __ne__(self, value):
        return not self == value

    def __lt__(self, value):
        if self.path != value.path:
            return self.path < value.path
        return self.net < value.net

    def __le__(self, value):
        if self.path != value.path:
            return self.path < value.path
        return self.net <= value.net

    def __gt__(self, value):
        if self.path != value.path:
            return self.path > value.path
        return self.net > value.net

    def __ge__(self, value):
        if self.path != value.path:
            return self.path > value.path
        return self.net >= value.net

    def __str__(self):
        return str(self.net)

    def get_name(self) -> str:
        return self.net.getName()

    def get_inst_terms(self):
        for term in self.net.getInstTerms():
            yield InstTerm(self.path, term)

    def get_top_terms(self):
        for term in self.net.getBitTerms():
            yield TopTerm(self.path, term)


class TopTerm:
    def __init__(self, term):
        self.term = term

    def __eq__(self, other) -> bool:
        return self.term == other.term

    def __ne__(self, other) -> bool:
        return not self == other

    def __lt__(self, other) -> bool:
        return self.term < other.term

    def __le__(self, other) -> bool:
        return self < other or self == other

    def __gt__(self, other) -> bool:
        return not self <= other

    def __ge__(self, other) -> bool:
        return not self < other

    def __str__(self) -> str:
        return str(self.term)

    def __repr__(self) -> str:
        return f"TopTerm({self.term})"

    def get_name(self) -> str:
        return self.term.getName()

    def get_direction(self) -> snl.SNLTerm.Direction:
        return self.term.getDirection()

    def get_net(self) -> Net:
        return Net(snl.SNLPath(), self.term.getNet())

    def get_equipotential(self) -> Equipotential:
        return Equipotential(self)

    def is_input(self) -> bool:
        return self.term.getDirection() == snl.SNLTerm.Direction.Input

    def is_output(self) -> bool:
        return self.term.getDirection() == snl.SNLTerm.Direction.Output


class InstTerm:
    def __init__(self, path, term):
        # assert inst exists in the path
        verify_instance_path(path, term.getInstance())
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

    def __str__(self) -> str:
        return str(
            snl.SNLInstTermOccurrence(self.path.getHeadPath(), self.term)
        )

    def __repr__(self) -> str:
        return f"InstTerm({self.path}, {self.term})"

    def get_name(self) -> str:
        return self.term.getName()

    def get_net(self) -> Net:
        return Net(self.path, self.term.getNet())

    def get_instance(self):
        return Instance(self.path, self.term.getInstance())

    def get_flat_fanout(self):
        return self.get_equipotential().get_all_leaf_readers()

    def get_equipotential(self) -> Equipotential:
        return Equipotential(self)

    def is_input(self) -> bool:
        return self.term.getDirection() == snl.SNLTerm.Direction.Input

    def is_output(self) -> bool:
        return self.term.getDirection() == snl.SNLTerm.Direction.Output

    def disconnect(self):
        term = self.term.getBitTerm()
        uniq = snl.SNLUniquifier(self.path)
        uniq_path = uniq.getPathUniqCollection()
        if len(tuple(uniq_path)) == self.path.size():
            inst = tuple(uniq_path)[len(tuple(uniq_path)) - 1]
            self.term = inst.getInstTerm(term)
        self.term.setNet(snl.SNLNet())

    def connect(self, net: Net):
        term = self.term.getBitTerm()
        uniq = snl.SNLUniquifier(self.path)
        uniq_path = uniq.getPathUniqCollection()
        if len(tuple(uniq_path)) == self.path.size():
            inst = tuple(uniq_path)[len(tuple(uniq_path)) - 1]
            self.term = inst.getInstTerm(term)
        self.term = inst.getInstTerm(term)
        self.term.setNet(net.net)


def verify_instance_path(path: snl.SNLPath, inst: snl.SNLInstance):
    pathlist = []
    pathTemp = path
    while pathTemp.size() > 0:
        pathlist.append(pathTemp.getHeadInstance().getName())
        pathTemp = pathTemp.getTailPath()
    assert len(pathlist) > 0
    path = snl.SNLPath()
    instance = None
    top = snl.SNLUniverse.get().getTopDesign()
    design = top
    for name in pathlist:
        path = snl.SNLPath(path, design.getInstance(name))
        instance = design.getInstance(name)
        assert instance is not None
        design = instance.getModel()
    assert inst == instance


def get_instance_by_path(names: list):
    assert len(names) > 0
    path = snl.SNLPath()
    instance = None
    top = snl.SNLUniverse.get().getTopDesign()
    design = top
    for name in names:
        path = snl.SNLPath(path, design.getInstance(name))
        instance = design.getInstance(name)
        assert instance is not None
        design = instance.getModel()
    return Instance(path, instance)


class Instance:
    """Class that represents the instance and wraps some
       of the snl occurrence API.
    """

    def __init__(self, path, inst):
        verify_instance_path(path, inst)
        self.inst = inst
        self.path = path

    def __eq__(self, other) -> bool:
        return self.inst == other.inst and self.path == other.path

    def __str__(self) -> str:
        return str(self.inst) + " " + str(self.path)

    def get_child_instance(self, name: str):
        childInst = self.inst.getModel().getInstance(name)
        if childInst is None:
            return None
        return Instance(
            snl.SNLPath(self.path, childInst), childInst)

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
            yield InstTerm(self.path, term)

    def get_inst_term(self, name: str) -> InstTerm:
        if self.inst is None:
            return None
        for term in self.inst.getInstTerms():
            if term.getBitTerm().getName() == name:
                return InstTerm(self.path, term)
        return None

    def is_primitive(self) -> bool:
        return self.inst.getModel().isPrimitive()

    def get_input_inst_terms(self):
        for term in self.inst.getInstTerms():
            if term.getDirection() == snl.SNLTerm.Direction.Input:
                yield InstTerm(self.path, term)

    def get_output_inst_terms(self):
        for term in self.inst.getInstTerms():
            if term.getDirection() == snl.SNLTerm.Direction.Output:
                yield InstTerm(self.path, term)

    def delete_instance(self, name: str):
        path = snl.SNLPath(self.path, self.inst.getModel().getInstance(name))
        uniq = snl.SNLUniquifier(path)
        uniq_path = uniq.getPathUniqCollection()
        self.inst = tuple(uniq_path)[len(tuple(uniq_path)) - 2]
        # Delete the last instance in uniq_path
        tuple(uniq_path)[len(tuple(uniq_path)) - 1].destroy()

    def get_name(self) -> str:
        return self.inst.getName()

    def get_model_name(self) -> str:
        return self.inst.getModel().getName()

    def create_child_instance(self, model, name):
        uniq = snl.SNLUniquifier(self.path)
        uniq_path = uniq.getPathUniqCollection()
        self.inst = tuple(uniq_path)[len(tuple(uniq_path)) - 1]
        design = self.inst.getModel()
        newSNLInstance = snl.SNLInstance.create(design, model, name)
        path = snl.SNLPath(self.path, newSNLInstance)
        return Instance(path, newSNLInstance)

    def create_output_term(self, name: str) -> InstTerm:
        uniq = snl.SNLUniquifier(self.path)
        uniq_path = uniq.getPathUniqCollection()
        self.inst = tuple(uniq_path)[len(tuple(uniq_path)) - 1]
        design = self.inst.getModel()
        newSNLTerm = snl.SNLScalarTerm.create(
            design,
            snl.SNLTerm.Direction.Output,
            name
        )
        newInstTerm = self.inst.getInstTerm(newSNLTerm)
        return InstTerm(self.path, newInstTerm)

    def create_input_term(self, name: str) -> InstTerm:
        uniq = snl.SNLUniquifier(self.path)
        uniq_path = uniq.getPathUniqCollection()
        self.inst = tuple(uniq_path)[len(tuple(uniq_path)) - 1]
        design = self.inst.getModel()
        newSNLTerm = snl.SNLScalarTerm.create(
            design,
            snl.SNLTerm.Direction.Input,
            name
        )
        newInstTerm = self.inst.getInstTerm(newSNLTerm)
        return InstTerm(self.path, newInstTerm)

    def create_output_bus_term(
        self,
        name: str,
        width: int,
        offset: int
    ) -> list:
        uniq = snl.SNLUniquifier(self.path)
        uniq_path = uniq.getPathUniqCollection()
        self.inst = tuple(uniq_path)[len(tuple(uniq_path)) - 1]
        design = self.inst.getModel()
        newSNLTerm = snl.SNLBusTerm.create(
            design,
            snl.SNLTerm.Direction.Output,
            width,
            offset,
            name
        )
        instTerms = []
        for i in range(width):
            instTerms.append(
                InstTerm(
                    self.path,
                    self.inst.getInstTerm(newSNLTerm.getBit(i))
                )
            )
        return instTerms

    def create_input_bus_term(
        self,
        name: str,
        width: int,
        offset: int
    ) -> list:
        uniq = snl.SNLUniquifier(self.path)
        uniq_path = uniq.getPathUniqCollection()
        self.inst = tuple(uniq_path)[len(tuple(uniq_path)) - 1]
        design = self.inst.getModel()
        newSNLTerm = snl.SNLBusTerm.create(
            design,
            snl.SNLTerm.Direction.Input,
            width,
            offset,
            name
        )
        instTerms = []
        for i in range(width):
            instTerms.append(
                InstTerm(
                    self.path,
                    self.inst.getInstTerm(newSNLTerm.getBit(i))
                )
            )
        return instTerms

    def create_net(self, name: str) -> Net:
        uniq = snl.SNLUniquifier(self.path)
        uniq_path = uniq.getPathUniqCollection()
        self.inst = tuple(uniq_path)[len(tuple(uniq_path)) - 1]
        model = self.inst.getModel()
        newSNLNet = snl.SNLScalarNet.create(model, name)
        return Net(self.path, newSNLNet)

    def create_bus_net(self, name: str, width: int, offset: int) -> list:
        uniq = snl.SNLUniquifier(self.path)
        uniq_path = uniq.getPathUniqCollection()
        self.inst = tuple(uniq_path)[len(tuple(uniq_path)) - 1]
        model = self.inst.getModel()
        newSNLNet = snl.SNLBusNet.create(model, width, offset, name)
        list = []
        for i in range(width):
            list.append(Net(self.path, newSNLNet.getBit(i)))
        return list

    def get_term_list_for_bus(self, name: str) -> list:
        list = []
        bus = self.inst.getModel().getBusTerm(name)
        for bit in bus.getBits():
            list.append(InstTerm(self.path, self.inst.getInstTerm(bit)))
        return list

    def get_net(self, name: str) -> Net:
        net = self.inst.getModel().getNet(name)
        if net is not None:
            return Net(self.path, net)
        return None

    def get_net_list_for_bus(self, name: str) -> list:
        list = []
        bus = self.inst.getModel().getBusNet(name)
        for bit in bus.getBits():
            list.append(Net(self.path, bit))
        return list


class Top:
    def __init__(self):
        self.design = snl.SNLUniverse.get().getTopDesign()

    def get_child_instance(self, name: str):
        return Instance(snl.SNLPath(), self.design.getInstance(name))

    def get_child_instances(self):
        for inst in self.design.getInstances():
            path = snl.SNLPath(snl.SNLPath(), inst)
            yield Instance(path, inst)

    def create_child_instance(self, model, name):
        design = getTop().design
        newSNLInstance = snl.SNLInstance.create(design, model, name)
        path = snl.SNLPath(self.path, newSNLInstance)
        return Instance(path, newSNLInstance)

    def delete_instance(self, name: str):
        getTop().design.getInstance(name).destroy()

    def create_output_term(self, name: str) -> InstTerm:
        design = getTop().design
        newSNLTerm = snl.SNLScalarTerm.create(
            design,
            snl.SNLTerm.Direction.Output,
            name
        )
        return TopTerm(newSNLTerm)

    def create_input_term(self, name: str) -> InstTerm:
        design = getTop().design
        newSNLTerm = snl.SNLScalarTerm.create(
            design,
            snl.SNLTerm.Direction.Input,
            name
        )
        return TopTerm(newSNLTerm)

    def create_output_bus_term(
        self,
        name: str,
        width: int,
        offset: int
    ) -> list:
        uniq = snl.SNLUniquifier(self.path)
        uniq_path = uniq.getPathUniqCollection()
        self.inst = tuple(uniq_path)[len(tuple(uniq_path)) - 1]
        design = self.inst.getModel()
        newSNLTerm = snl.SNLBusTerm.create(
            design,
            snl.SNLTerm.Direction.Output,
            width,
            offset,
            name
        )
        instTerms = []
        for i in range(width):
            instTerms.append(
                TopTerm(
                    self.path,
                    self.inst.getInstTerm(newSNLTerm.getBit(i))
                )
            )
        return instTerms

    def create_input_bus_term(
        self, name: str,
        width: int,
        offset: int
    ) -> list:
        uniq = snl.SNLUniquifier(self.path)
        uniq_path = uniq.getPathUniqCollection()
        self.inst = tuple(uniq_path)[len(tuple(uniq_path)) - 1]
        design = self.inst.getModel()
        newSNLTerm = snl.SNLBusTerm.create(
            design,
            snl.SNLTerm.Direction.Input,
            width,
            offset,
            name
        )
        instTerms = []
        for i in range(width):
            instTerms.append(
                InstTerm(
                    self.path,
                    self.inst.getInstTerm(newSNLTerm.getBit(i))
                )
            )
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
            list.append(Net(self.path, newSNLNet.getBit(i)))
        return list

    def get_term_list_for_bus(self, name: str) -> list:
        list = []
        bus = self.inst.getModel().getBusTerm(name)
        for bit in bus.getBits():
            list.append(InstTerm(self.path, self.inst.getInstTerm(bit)))
        return list

    def get_net(self, name: str) -> Net:
        for term in self.inst.getInstTerms():
            if term.getBitTerm().getName() == name:
                return Net(self.path, term.getNet())
        return None

    def get_net_list_for_bus(self, name: str) -> list:
        list = []
        bus = self.inst.getModel().getBusNet(name)
        for bit in bus.getBits():
            list.append(Net(self.path, bit))
        return list


def getTopDB() -> snl.SNLDB:
    if snl.SNLUniverse.get() is None:
        snl.SNLUniverse.create()
    if snl.SNLUniverse.get().getTopDB() is None:
        db = snl.SNLDB.create(snl.SNLUniverse.get())
        snl.SNLUniverse.get().setTopDB(db)
    return snl.SNLUniverse.get().getTopDB()


def getTop():
    return Top()


def createTop():
    # init
    db = getTopDB()
    # create top design
    lib = snl.SNLLibrary.create(db)
    top = snl.SNLDesign.create(lib)
    snl.SNLUniverse.get().setTopDesign(top)
    return Top()


def load_verilog(files: list):
    getTopDB().loadVerilog(files)
    return getTop()

def load_liberty(files: list):
    getTopDB().loadLibertyPrimitives(files)


def get_primitives_library() -> snl.SNLLibrary:
    lib = getTopDB().getLibrary("PRIMS")
    if lib is None:
        lib = snl.SNLLibrary.createPrimitives(getTopDB())
    return lib


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
