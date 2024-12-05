# SPDX-FileCopyrightText: 2024 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

from najaeda import snl


class Equipotential:
    """Class that represents the term and wraps
       some of the snl occurrence API.
    """

    def __init__(self, term):
        ito = snl.SNLNetComponentOccurrence(
            term.path.getHeadPath(),
            term.path.getTailInstance().getInstTerm(term.term)
        )
        self.equi = snl.SNLEquipotential(ito)

    def __eq__(self, value):
        return self.equi == value.equi

    def get_inst_terms(self):
        for term in self.equi.getInstTermOccurrences():
            yield Term(
                snl.SNLPath(term.getPath(), term.getInstTerm().getInstance()),
                term.getInstTerm().getBitTerm()
            )

    def get_top_terms(self):
        for term in self.equi.getTerms():
            yield Term(snl.SNLPath(), term.getBitTerm())

    def get_all_leaf_readers(self):
        for term in self.equi.getInstTermOccurrences():
            direction = term.getTerm().getDirection()
            if direction == snl.SNLTerm.Direction.Output:
                yield Term(
                    snl.SNLPath(
                        term.getPath(),
                        term.getInstTerm().getInstance()
                    ),
                    term.getInstTerm().getBitTerm()
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
            yield Term(self.path, term.getBitTerm())

    def get_model_terms(self):
        for term in self.net.getBitTerms():
            yield Term(self.path, term)


class Term:
    def __init__(self, path, term):
        # assert inst exists in the path
        if (path.size() > 0):
            #print(term)
            #print(path.getTailInstance())
            #print(path.getTailInstance().getInstTerm(term))
            assert path.getTailInstance().getInstTerm(term) is not None
        else:
            top = snl.SNLUniverse.get().getTopDesign()
            assert top.getTerm(term.getName()) is not None
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
        return str(self.path) + "." + self.term.getName()

    def __repr__(self) -> str:
        return f"Term({self.path}, {self.term})"

    def get_name(self) -> str:
        return self.term.getName()

    def get_net(self) -> Net:
        net = None
        if self.path.size() > 0:
            instTerm = self.path.getTailInstance().getInstTerm(self.term)
            net = instTerm.getNet()
        else:
            net = get_top().model.getTerm(self.term.getName()).getNet()
        return Net(self.path, net)

    def get_instance(self):
        return Instance(self.path)

    def get_flat_fanout(self):
        return self.get_equipotential().get_all_leaf_readers()

    def get_equipotential(self) -> Equipotential:
        return Equipotential(self)

    def is_input(self) -> bool:
        return self.term.getDirection() == snl.SNLTerm.Direction.Input

    def is_output(self) -> bool:
        return self.term.getDirection() == snl.SNLTerm.Direction.Output

    def disconnect(self):
        term = self.term
        if self.path.size() > 0:
            uniq = snl.SNLUniquifier(self.path)
            uniq_path = uniq.getPathUniqCollection()
            inst = tuple(uniq_path)[len(tuple(uniq_path)) - 1]
            self.term = inst.getInstTerm(term).getBitTerm()
        termToConnect = inst.getInstTerm(term)
        termToConnect.setNet(snl.SNLNet())

    def connect(self, net: Net):
        term = self.term
        if self.path.size() > 0:
            uniq = snl.SNLUniquifier(self.path)
            uniq_path = uniq.getPathUniqCollection()
            inst = tuple(uniq_path)[len(tuple(uniq_path)) - 1]
            self.term = inst.getInstTerm(term).getBitTerm()
        termToConnect = inst.getInstTerm(term)
        termToConnect.setNet(net.net)


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
    return Instance(path)


def get_path_for_names(names: list):
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
    return path


def refresh_path(path: snl.SNLPath):
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
    return path


class Instance:
    """Class that represents the instance and wraps some
       of the snl occurrence API.
    """

    def __init__(self, path=snl.SNLPath()):
        self.path = path

    def __eq__(self, other) -> bool:
        return self.path == other.path

    def __str__(self) -> str:
        return str(self.path)

    def __repr__(self) -> str:
        return f"Instance({self.path})"

    def is_top(self) -> bool:
        return self.path.size() == 0

    def get_model(self):
        if self.is_top():
            return snl.SNLUniverse.get().getTopDesign()
        return self.path.getTailInstance().getModel()

    def get_child_instance(self, name: str):
        childInst = self.get_model().getInstance(name)
        if childInst is None:
            return None
        return Instance(snl.SNLPath(self.path, childInst))

    def get_child_instances(self):
        for inst in self.get_model().getInstances():
            path = snl.SNLPath(self.path, inst)
            yield Instance(path)

    def get_number_of_child_instances(self) -> int:
        return len(tuple(self.get_model().getInstances()))

    def get_terms(self):
        for term in self.get_model().getBitTerms():
            yield Term(self.path, term)

    def get_term(self, name: str) -> Term:
        for term in self.get_model().getBitTerms():
            if term.getName() == name:
                return Term(self.path, term)
        return None

    def is_primitive(self) -> bool:
        return self.get_model().isPrimitive()

    def get_input_terms(self):
        for term in self.get_model().getBitTerms():
            if term.getDirection() == snl.SNLTerm.Direction.Input:
                yield Term(self.path, term)

    def get_output_terms(self):
        for term in self.get_model().getBitTerms():
            if term.getDirection() == snl.SNLTerm.Direction.Output:
                yield Term(self.path, term)

    def delete_instance(self, name: str):
        path = snl.SNLPath(self.path, self.get_model().getInstance(name))
        uniq = snl.SNLUniquifier(path)
        if self.path.size() > 1:
            self.path = refresh_path(self.path)
            self.model = tuple(uniq.getPathUniqCollection())[len(
                tuple(uniq.getPathUniqCollection())) - 2].getModel()
            self.path = refresh_path(self.path)
        # Delete the last instance in uniq_path
        self.get_model().getInstance(name).destroy()

    def get_name(self) -> str:
        return self.path.getTailInstance().getName()

    def get_model_name(self) -> str:
        return self.get_model().getName()

    def create_child_instance(self, model, name):
        if self.path.size() > 0:
            path = self.path
            uniq = snl.SNLUniquifier(path)
            self.path = refresh_path(self.path)
            self.model = tuple(uniq.getPathUniqCollection())[len(
                tuple(uniq.getPathUniqCollection())) - 1].getModel()
            self.path = refresh_path(self.path)
        design = self.model
        newSNLInstance = snl.SNLInstance.create(design, model, name)
        path = snl.SNLPath(self.path, newSNLInstance)
        return Instance(path)

    def create_term(self, name: str, direction: snl.SNLTerm.Direction) -> Term:
        if self.path.size() > 0:
            path = self.path
            uniq = snl.SNLUniquifier(path)
            self.path = refresh_path(self.path)
            self.model = tuple(uniq.getPathUniqCollection())[len(
                tuple(uniq.getPathUniqCollection())) - 1].getModel()
            self.path = refresh_path(self.path)
        design = self.get_model()
        newSNLTerm = snl.SNLScalarTerm.create(
            design,
            direction,
            name
        )
        return Term(self.path, newSNLTerm)

    def create_output_term(self, name: str) -> Term:
        return self.create_term(name, snl.SNLTerm.Direction.Output)

    def create_input_term(self, name: str) -> Term:
        return self.create_term(name, snl.SNLTerm.Direction.Input)

    def create_output_bus_term(
        self,
        name: str,
        width: int,
        offset: int
    ) -> list:
        if self.path.size() > 0:
            path = self.path
            uniq = snl.SNLUniquifier(path)
            self.path = refresh_path(self.path)
            self.model = tuple(uniq.getPathUniqCollection())[len(
                tuple(uniq.getPathUniqCollection())) - 1].getModel()
            self.path = refresh_path(self.path)
        design = self.model
        newSNLTerm = snl.SNLBusTerm.create(
            design,
            snl.SNLTerm.Direction.Output,
            width,
            offset,
            name
        )
        busTerms = []
        for i in range(width):
            busTerms.append(
                Term(
                    self.path,
                    newSNLTerm.getBit(i)))
        return busTerms

    def create_input_bus_term(
        self,
        name: str,
        width: int,
        offset: int
    ) -> list:
        if self.path.size() > 0:
            path = self.path
            uniq = snl.SNLUniquifier(path)
            self.path = refresh_path(self.path)
            self.model = tuple(uniq.getPathUniqCollection())[len(
                tuple(uniq.getPathUniqCollection())) - 1].getModel()
            self.path = refresh_path(self.path)
        design = self.get_model()
        newSNLTerm = snl.SNLBusTerm.create(
            design,
            snl.SNLTerm.Direction.Input,
            width,
            offset,
            name
        )
        busTerms = []
        for i in range(width):
            busTerms.append(
                Term(
                    self.path,
                    newSNLTerm.getBit(i)))
        return busTerms

    def create_net(self, name: str) -> Net:
        if self.path.size() > 0:
            path = self.path
            uniq = snl.SNLUniquifier(path)
            self.path = refresh_path(self.path)
            self.model = tuple(uniq.getPathUniqCollection())[len(
                tuple(uniq.getPathUniqCollection())) - 1].getModel()
            self.path = refresh_path(self.path)
        model = self.get_model()
        newSNLNet = snl.SNLScalarNet.create(model, name)
        return Net(self.path, newSNLNet)

    def create_bus_net(self, name: str, width: int, offset: int) -> list:
        if self.path.size() > 0:
            path = self.path
            uniq = snl.SNLUniquifier(path)
            self.path = refresh_path(self.path)
            self.model = tuple(uniq.getPathUniqCollection())[len(
                tuple(uniq.getPathUniqCollection())) - 1].getModel()
            self.path = refresh_path(self.path)
        model = self.get_model()
        newSNLNet = snl.SNLBusNet.create(model, width, offset, name)
        list = []
        for i in range(width):
            list.append(Net(self.path, newSNLNet.getBit(i)))
        return list

    def get_term_list_for_bus(self, name: str) -> list:
        list = []
        bus = self.get_model().getBusTerm(name)
        for bit in bus.getBits():
            list.append(Term(self.path, bit))
        return list

    def get_net(self, name: str) -> Net:
        net = self.get_model().getNet(name)
        if net is not None:
            return Net(self.path, net)
        return None

    def get_net_list_for_bus(self, name: str) -> list:
        list = []
        bus = self.get_model().getBusNet(name)
        for bit in bus.getBits():
            list.append(Net(self.path, bit))
        return list


def get_top_db() -> snl.SNLDB:
    if snl.SNLUniverse.get() is None:
        snl.SNLUniverse.create()
    if snl.SNLUniverse.get().getTopDB() is None:
        db = snl.SNLDB.create(snl.SNLUniverse.get())
        snl.SNLUniverse.get().setTopDB(db)
    return snl.SNLUniverse.get().getTopDB()


def get_top():
    return Instance(snl.SNLPath())


def create_top():
    # init
    db = get_top_db()
    # create top design
    lib = snl.SNLLibrary.create(db)
    top = snl.SNLDesign.create(lib)
    snl.SNLUniverse.get().setTopDesign(top)
    return Instance()


def load_verilog(files: list):
    get_top_db().loadVerilog(files)
    return get_top()


def load_liberty(files: list):
    get_top_db().loadLibertyPrimitives(files)


def get_primitives_library() -> snl.SNLLibrary:
    lib = get_top_db().getLibrary("PRIMS")
    if lib is None:
        lib = snl.SNLLibrary.createPrimitives(get_top_db())
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
                    primitives.append(Instance(path_child))
                stack.append([inst_child, path_child])
    return primitives
