# SPDX-FileCopyrightText: 2024 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import itertools
import logging

from najaeda import snl

class Equipotential:
    """Class that represents the term and wraps
    some of the snl occurrence API.
    """

    def __init__(self, term):
        if isinstance(term.term, snl.SNLBusTerm):
            raise ValueError("Equipotential cannot be constructed on bus term")
        ito = snl.SNLNetComponentOccurrence(
            term.path.getHeadPath(), term.path.getTailInstance().getInstTerm(term.term)
        )
        self.equi = snl.SNLEquipotential(ito)

    def __eq__(self, value):
        return self.equi == value.equi

    def get_inst_terms(self):
        for term in self.equi.getInstTermOccurrences():
            yield Term(
                snl.SNLPath(term.getPath(), term.getInstTerm().getInstance()),
                term.getInstTerm().getBitTerm(),
            )

    def get_top_terms(self):
        for term in self.equi.getTerms():
            yield Term(snl.SNLPath(), term.getBitTerm())

    def get_all_leaf_readers(self):
        for term in self.equi.getInstTermOccurrences():
            direction = term.getInstTerm().getDirection()
            if direction != snl.SNLTerm.Direction.Output:
                if term.getInstTerm().getInstance().getModel().isPrimitive():
                    yield Term(
                        snl.SNLPath(term.getPath(), term.getInstTerm().getInstance()),
                        term.getInstTerm().getBitTerm(),
                    )


class Net:
    def __init__(self, path, net=None, net_concat=None):
        if net is not None and net_concat is not None:
            raise ValueError(
                "Only one of `net` or `net_concat` should be provided, not both."
            )
        self.path = path
        if net is not None:
            self.net = net
        elif net_concat is not None:
            self.net_concat = net_concat

    def __eq__(self, other):
        if not isinstance(other, Net):
            return NotImplemented
        return vars(self) == vars(other)

    def __ne__(self, other):
        eq_result = self.__eq__(other)
        if eq_result is NotImplemented:
            return NotImplemented
        return not eq_result

    def __str__(self):
        if hasattr(self, "net"):
            net_str = str(self.net)
        elif hasattr(self, "net_concat"):
            net_str = "{" + ",".join(map(str, self.net_concat)) + "}"
        if self.path.size() > 0:
            return f"{self.path}/{net_str}"
        return net_str

    def get_name(self) -> str:
        """Return the name of the net."""
        if hasattr(self, "net"):
            return self.net.getName()
        return "{" + ",".join(map(str, self.net_concat)) + "}"

    def get_msb(self) -> int:
        """Return the most significant bit of the net if it is a bus."""
        if hasattr(self, "net") and isinstance(self.net, snl.SNLBusNet):
            return self.net.getMSB()
        return None

    def get_lsb(self) -> int:
        """Return the least significant bit of the net if it is a bus."""
        if hasattr(self, "net") and isinstance(self.net, snl.SNLBusNet):
            return self.net.getLSB()
        return None

    def is_bus(self) -> bool:
        """Return True if the net is a bus."""
        return hasattr(self, "net") and isinstance(self.net, snl.SNLBusNet)

    def is_bus_bit(self) -> bool:
        """Return True if the net is a bit of a bus."""
        return hasattr(self, "net") and isinstance(self.net, snl.SNLBusNetBit)

    def is_scalar(self) -> bool:
        """Return True if the net is a scalar."""
        return hasattr(self, "net") and isinstance(self.net, snl.SNLScalarNet)

    def is_bit(self) -> bool:
        """Return True if the net is a bit."""
        return self.is_scalar() or self.is_bus_bit()

    def is_concat(self) -> bool:
        """Return True if the net is a concatenation."""
        return hasattr(self, "net_concat")

    def is_constant(self) -> bool:
        """Return True if the net is a constant generator."""
        if hasattr(self, "net"):
            return self.net.isConstant()
        for net in self.net_concat:
            if not net.is_constant():
                return False
        return True

    def get_width(self) -> int:
        """Return the width of the net."""
        if hasattr(self, "net"):
            return self.net.getWidth()
        return sum(1 for _ in self.net_concat)

    def get_bits(self):
        if hasattr(self, "net"):
            if isinstance(self.net, snl.SNLBusNet):
                for bit in self.net.getBits():
                    yield Net(self.path, bit)
            else:
                yield self
        else:
            for net in self.net_concat:
                yield net

    def get_bit(self, index: int):
        if hasattr(self, "net"):
            if isinstance(self.net, snl.SNLBusNet):
                return Net(self.path, self.net.getBit(index))
            else:
                return None
        if 0 <= index < len(self.net_concat):
            return Net(self.path, self.net_concat[index])
        return None

    def get_inst_terms(self):
        if hasattr(self, "net_concat"):
            raise ValueError("Cannot get inst terms from a net_concat")
        for term in self.net.getInstTerms():
            path = snl.SNLPath(self.path, term.getInstance())
            yield Term(path, term.getBitTerm())

    def get_terms(self):
        if hasattr(self, "net_concat"):
            raise ValueError("Cannot get terms from a net_concat")
        for term in self.net.getBitTerms():
            yield Term(self.path, term)

    def get_components(self):
        for term in itertools.chain(self.get_terms(), self.get_inst_terms()):
            yield term


class Term:
    Input = snl.SNLTerm.Direction.Input
    Output = snl.SNLTerm.Direction.Output
    InOut = snl.SNLTerm.Direction.InOut

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

    def __str__(self):
        if self.path.size() == 0:
            return self.term.getName()
        else:
            return f"{self.path}/{self.term}"

    def __repr__(self) -> str:
        return f"Term({self.path}, {self.term})"

    def __make_unique(self):
        if self.path.size() > 1:
            path = self.path.getHeadPath()
            snl.SNLUniquifier(path)
            if self.is_bus_bit():
                term = (
                    self.path.getTailInstance().getModel().getTerm(self.term.getName())
                )
                self.term = term.getBit(self.term.getBit())
            else:
                self.term = (
                    self.path.getTailInstance().getModel().getTerm(self.term.getName())
                )

    def is_bus(self) -> bool:
        """Return True if the term is a bus."""
        return isinstance(self.term, snl.SNLBusTerm)

    def is_bus_bit(self) -> bool:
        """Return True if the term is a bit of a bus."""
        return isinstance(self.term, snl.SNLBusTermBit)

    def is_scalar(self) -> bool:
        """Return True if the term is a scalar."""
        return isinstance(self.term, snl.SNLScalarTerm)

    def is_bit(self) -> bool:
        """Return True if the term is a bit."""
        return self.is_scalar() or self.is_bus_bit()

    def get_msb(self) -> int:
        """Return the most significant bit of the term if it is a bus."""
        if isinstance(self.term, snl.SNLBusTerm):
            return self.term.getMSB()
        return None

    def get_lsb(self) -> int:
        """Return the least significant bit of the term if it is a bus."""
        if isinstance(self.term, snl.SNLBusTerm):
            return self.term.getLSB()
        return None

    def get_width(self) -> int:
        """Return the width of the term. 1 if scalar."""
        return self.term.getWidth()

    def get_name(self) -> str:
        """Return the name of the term."""
        return self.term.getName()

    def get_direction(self) -> snl.SNLTerm.Direction:
        """Return the direction of the term."""
        if self.term.getDirection() == snl.SNLTerm.Direction.Input:
            return Term.Input
        elif self.term.getDirection() == snl.SNLTerm.Direction.Output:
            return Term.Output
        elif self.term.getDirection() == snl.SNLTerm.Direction.InOut:
            return Term.InOut

    def __get_snl_bitnet(self, bit) -> Net:
        # single bit
        if self.path.size() > 0:
            instTerm = self.path.getTailInstance().getInstTerm(bit)
            return instTerm.getNet()
        else:
            return bit.getNet()

    def __get_snl_lower_bitnet(self, bit) -> Net:
        return bit.getNet()

    def __get_snl_busnet(self, snl_nets) -> snl.SNLBusNet:
        # iterate on all elements of the list and check if
        # a full SNLBusNet can be reconstructed
        snl_bus_net = None
        for i in range(len(snl_nets)):
            snl_net = snl_nets[i]
            if not isinstance(snl_net, snl.SNLBusNetBit):
                return None
            bit_bus = snl_net.getBus()
            if bit_bus.getWidth() != len(snl_nets):
                return None
            if snl_bus_net is None:
                snl_bus_net = bit_bus
            if snl_bus_net != bit_bus:
                return None
            if snl_bus_net.getBitAtPosition(i) != snl_net:
                return None
        return snl_bus_net

    def __get_net(self, path, snl_term_net_accessor) -> Net:
        if isinstance(self.term, snl.SNLBusTerm):
            snl_nets = []
            for bit in self.term.getBits():
                snl_net = snl_term_net_accessor(bit)
                snl_nets.append(snl_net)
            snl_bus_net = self.__get_snl_busnet(snl_nets)
            if snl_bus_net is not None:
                return Net(path, snl_bus_net)
            else:
                if all(element is not None for element in snl_nets):
                    return Net(path, net_concat=snl_nets)
        else:
            snl_net = snl_term_net_accessor(self.term)
            if snl_net is not None:
                return Net(path, snl_net)
        return None
        
    def get_lower_net(self) -> Net:
        """Return the lower net of the term."""
        return self.__get_net(self.path, self.__get_snl_lower_bitnet)

    def get_net(self) -> Net:
        """Return the net of the term."""
        if self.path.empty():
            return None
        # path is one level up
        path = self.path.getHeadPath()
        return self.__get_net(path, self.__get_snl_bitnet)

    def get_instance(self):
        """Return the instance of the term."""
        return Instance(self.path)

    def get_flat_fanout(self):
        return self.get_equipotential().get_all_leaf_readers()

    def get_equipotential(self) -> Equipotential:
        return Equipotential(self)

    def is_input(self) -> bool:
        """Return True if the term is an input."""
        return self.term.getDirection() == snl.SNLTerm.Direction.Input

    def is_output(self) -> bool:
        """Return True if the term is an output."""
        return self.term.getDirection() == snl.SNLTerm.Direction.Output

    def get_bits(self):
        if isinstance(self.term, snl.SNLBusTerm):
            for bit in self.term.getBits():
                yield Term(self.path, bit)
        else:
            yield self

    def get_bit(self, index: int):
        if isinstance(self.term, snl.SNLBusTerm):
            return Term(self.path, self.term.getBit(index))
        return None

    def disconnect(self):
        self.__make_unique()
        inst = self.path.getTailInstance()
        for bit in self.term.getBits():
            iterm = inst.getInstTerm(bit)
            iterm.setNet(None)

    def connect(self, net: Net):
        if self.get_width() != net.get_width():
            raise ValueError("Width mismatch")
        if self.get_instance().is_top():
            for bterm, bnet in zip(self.term.getBits(), net.net.getBits()):
                logging.debug(f"Connecting {bterm} to {bnet}")
                bterm.setNet(bnet)
        else:
            self.__make_unique()
            inst = self.path.getTailInstance()
            for bterm, bnet in zip(self.term.getBits(), net.net.getBits()):
                iterm = inst.getInstTerm(bterm)
                iterm.setNet(bnet)


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

    def __str__(self):
        if self.is_top():
            top = self.__get_snl_model()
            if top is not None:
                return top.getName()
            else:
                return ""
        else:
            return str(self.path)

    def __repr__(self) -> str:
        return f"Instance({self.path})"

    def __hash__(self):
        return hash(self.path)

    def is_top(self) -> bool:
        """Return True if this is the top design."""
        return self.path.size() == 0

    def is_assign(self) -> bool:
        return self.__get_snl_model().isAssign()

    def is_blackbox(self) -> bool:
        """Return True if this is a blackbox."""
        return self.__get_snl_model().isBlackBox()

    def is_const0(self) -> bool:
        """Return True if this is a constant 0 generator."""
        return self.__get_snl_model().isConst0()

    def is_const1(self) -> bool:
        """Return True if this is a constant 1 generator."""
        return self.__get_snl_model().isConst1()

    def is_const(self) -> bool:
        """Return True if this is a constant generator."""
        return self.__get_snl_model().isConst()

    def is_buf(self) -> bool:
        """Return True if this is a buffer."""
        return self.__get_snl_model().isBuf()

    def is_inv(self) -> bool:
        """Return True if this is an inverter."""
        return self.__get_snl_model().isInv()

    def __get_snl_model(self):
        """Return the model of the instance."""
        if self.is_top():
            return snl.SNLUniverse.get().getTopDesign()
        return self.path.getTailInstance().getModel()

    def __find_snl_model(self, name: str) -> snl.SNLDesign:
        u = snl.SNLUniverse.get()
        if u is None:
            return None
        for db in u.getUserDBs():
            for lib in db.getLibraries():
                found_model = lib.getDesign(name)
                if found_model is not None:
                    return found_model
        return None

    def get_child_instance(self, name: str):
        childInst = self.__get_snl_model().getInstance(name)
        if childInst is None:
            return None
        return Instance(snl.SNLPath(self.path, childInst))

    def get_child_instances(self):
        for inst in self.__get_snl_model().getInstances():
            path = snl.SNLPath(self.path, inst)
            yield Instance(path)

    def get_number_of_child_instances(self) -> int:
        return sum(1 for _ in self.__get_snl_model().getInstances())

    def get_terms(self):
        for term in self.__get_snl_model().getTerms():
            yield Term(self.path, term)

    def get_term(self, name: str) -> Term:
        term = self.__get_snl_model().getTerm(name)
        if term is not None:
            return Term(self.path, self.__get_snl_model().getTerm(name))
        return None

    def get_nets(self):
        for net in self.__get_snl_model().getNets():
            yield Net(self.path, net)

    def get_flat_nets(self):
        for net in self.__get_snl_model().getNets():
            if isinstance(net, snl.SNLBusNet):
                for bit in net.getBits():
                    yield Net(self.path, bit)
            else:
                yield Net(self.path, net)

    def get_net(self, name: str) -> Net:
        net = self.__get_snl_model().getNet(name)
        if net is not None:
            return Net(self.path, net)
        return None

    def is_primitive(self) -> bool:
        """Return True if this is a primitive."""
        return self.__get_snl_model().isPrimitive()

    def get_input_terms(self):
        for term in self.__get_snl_model().getTerms():
            if term.getDirection() == snl.SNLTerm.Direction.Input:
                yield Term(self.path, term)

    def get_flat_input_terms(self):
        for term in self.__get_snl_model().getTerms():
            if term.getDirection() == snl.SNLTerm.Direction.Input:
                if isinstance(term, snl.SNLBusTerm):
                    for bit in term.getBits():
                        yield Term(self.path, bit)
                else:
                    yield Term(self.path, term)

    def get_output_terms(self):
        for term in self.__get_snl_model().getTerms():
            if term.getDirection() == snl.SNLTerm.Direction.Output:
                yield Term(self.path, term)

    def get_flat_output_terms(self):
        for term in self.__get_snl_model().getTerms():
            if term.getDirection() == snl.SNLTerm.Direction.Output:
                if isinstance(term, snl.SNLBusTerm):
                    for bit in term.getBits():
                        yield Term(self.path, bit)
                else:
                    yield Term(self.path, term)

    def delete_instance(self, name: str):
        path = snl.SNLPath(self.path, self.__get_snl_model().getInstance(name))
        snl.SNLUniquifier(path)
        if self.path.size() > 0:
            self.path = refresh_path(self.path)
        # Delete the last instance in uniq_path
        self.__get_snl_model().getInstance(name).destroy()

    def get_name(self) -> str:
        """Return the name of the instance or name of the top is this is the top."""
        if self.is_top():
            return self.get_model_name()
        else:
            return self.path.getTailInstance().getName()

    def get_model_name(self) -> str:
        """Return the name of the model of the instance or name of the top is this is the top."""
        return self.__get_snl_model().getName()

    def get_model_id(self) -> tuple[int, int, int]:
        model = self.__get_snl_model()
        return model.getDB().getID(), model.getLibrary().getID(), model.getID()

    def create_child_instance(self, model: str, name: str):
        if self.path.size() > 0:
            path = self.path
            snl.SNLUniquifier(path)
            self.path = refresh_path(self.path)
        design = self.__get_snl_model()
        new_instance_model = self.__find_snl_model(model)
        if new_instance_model is None:
            raise ValueError(
                f"Cannot create instance {name} in {self}: model {model} cannot be found"
            )
        newSNLInstance = snl.SNLInstance.create(design, new_instance_model, name)
        path = snl.SNLPath(self.path, newSNLInstance)
        return Instance(path)

    def create_term(self, name: str, direction: snl.SNLTerm.Direction) -> Term:
        if self.path.size() > 0:
            path = self.path
            snl.SNLUniquifier(path)
            self.path = refresh_path(self.path)
        design = self.__get_snl_model()
        newSNLTerm = snl.SNLScalarTerm.create(design, direction, name)
        return Term(self.path, newSNLTerm)

    def create_output_term(self, name: str) -> Term:
        return self.create_term(name, snl.SNLTerm.Direction.Output)

    def create_input_term(self, name: str) -> Term:
        return self.create_term(name, snl.SNLTerm.Direction.Input)

    def create_bus_term(
        self, name: str, msb: int, lsb: int, direction: snl.SNLTerm.Direction
    ) -> Term:
        if self.path.size() > 0:
            path = self.path
            snl.SNLUniquifier(path)
            self.path = refresh_path(self.path)
        design = self.__get_snl_model()
        newSNLTerm = snl.SNLBusTerm.create(design, direction, msb, lsb, name)
        return Term(self.path, newSNLTerm)

    def create_output_bus_term(self, name: str, msb: int, lsb: int) -> Term:
        return self.create_bus_term(name, msb, lsb, snl.SNLTerm.Direction.Output)

    def create_input_bus_term(self, name: str, msb: int, lsb: int) -> Term:
        return self.create_bus_term(name, msb, lsb, snl.SNLTerm.Direction.Input)

    def create_net(self, name: str) -> Net:
        if self.path.size() > 0:
            path = self.path
            snl.SNLUniquifier(path)
            self.path = refresh_path(self.path)
        model = self.__get_snl_model()
        newSNLNet = snl.SNLScalarNet.create(model, name)
        return Net(self.path, newSNLNet)

    def create_bus_net(self, name: str, msb: int, lsb: int) -> Net:
        if self.path.size() > 0:
            path = self.path
            snl.SNLUniquifier(path)
            self.path = refresh_path(self.path)
        model = self.__get_snl_model()
        newSNLNet = snl.SNLBusNet.create(model, msb, lsb, name)
        return Net(self.path, newSNLNet)

    def dump_verilog(self, path: str, name: str):
        self.__get_snl_model().dumpVerilog(path, name)


def get_top_db() -> snl.SNLDB:
    if snl.SNLUniverse.get() is None:
        snl.SNLUniverse.create()
    if snl.SNLUniverse.get().getTopDB() is None:
        db = snl.SNLDB.create(snl.SNLUniverse.get())
        snl.SNLUniverse.get().setTopDB(db)
    return snl.SNLUniverse.get().getTopDB()


def get_top():
    return Instance(snl.SNLPath())


def create_top(name: str) -> Instance:
    # init
    db = get_top_db()
    # create top design
    lib = snl.SNLLibrary.create(db)
    top = snl.SNLDesign.create(lib, name)
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


def get_model_name(id: tuple[int, int, int]) -> str:
    """Return the name of the model given its id."""
    u = snl.SNLUniverse.get()
    if u:
        db = u.getDB(id[0])
        if db:
            lib = db.getLibrary(id[1])
            if lib:
                model = lib.getDesign(id[2])
                if model:
                    return model.getName()
    return None


def get_all_primitive_instances():
    top = snl.SNLUniverse.get().getTopDesign()

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
                    yield Instance(path_child)
                stack.append([inst_child, path_child])
