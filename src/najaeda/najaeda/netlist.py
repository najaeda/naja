# SPDX-FileCopyrightText: 2024 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import itertools
import time
import logging
import hashlib
# import json
from najaeda import snl
import struct


def consistent_hash(obj):
    def default_serializer(o):
        if isinstance(o, (str, int, float, bool, type(None))):
            return o
        elif isinstance(o, (list, tuple)):
            return [default_serializer(i) for i in o]
        else:
            return str(o)

    def hash_value(value):
        if isinstance(value, int):
            return struct.pack('!q', value)
        else:
            raise TypeError(f"Unsupported type: {type(value)}")

    def hash_object(o):
        if isinstance(o, (list, tuple)):
            return b''.join(hash_object(i) for i in o)
        else:
            return hash_value(o)
    serialized_obj = default_serializer(obj)
    obj_bytes = hash_object(serialized_obj)
    return int(hashlib.sha256(obj_bytes).hexdigest(), 16)


def get_snl_instance_from_id_list(id_list: list) -> snl.SNLInstance:
    top = snl.SNLUniverse.get().getTopDesign()
    design = top
    instance = None
    for id in id_list:
        instance = design.getInstanceByID(id)
        assert instance is not None
        design = instance.getModel()
    return instance


def get_snl_path_from_id_list(id_list: list) -> snl.SNLPath:
    top = snl.SNLUniverse.get().getTopDesign()
    design = top
    path = snl.SNLPath()
    for id in id_list:
        instance = design.getInstanceByID(id)
        assert instance is not None
        path = snl.SNLPath(path, instance)
        assert path.getTailInstance() is not None
        design = instance.getModel()
    if len(id_list) > 0:
        assert path.getTailInstance() is not None
    return path


class Equipotential:
    """Class that represents the term and wraps
    some of the snl occurrence API.
    """

    def __init__(self, term):
        snl_term = get_snl_term_for_ids(term.pathIDs, term.termIDs)
        inst_term = None
        if isinstance(snl_term, snl.SNLBusTerm):
            raise ValueError("Equipotential cannot be constructed on bus term")
        if len(term.pathIDs) == 0:
            net = term.get_lower_net()
            if net is None:
                self.equi = None
                return
            inst_term = next(net.get_inst_terms(), None)
            if inst_term is None:
                self.equi = None
                return
            else:
                snl_term = get_snl_term_for_ids(inst_term.pathIDs, inst_term.termIDs)
        else:
            inst_term = term
        path = get_snl_path_from_id_list(inst_term.pathIDs)
        ito = snl.SNLNetComponentOccurrence(
            path.getHeadPath(),
            path.getTailInstance().getInstTerm(snl_term)
        )
        self.equi = snl.SNLEquipotential(ito)

    def __eq__(self, value):
        return self.equi == value.equi

    def get_inst_terms(self):
        if self.equi is not None:
            for term in self.equi.getInstTermOccurrences():
                yield Term(
                    snl.SNLPath(term.getPath(), term.getInstTerm().getInstance()),
                    term.getInstTerm().getBitTerm(),
                )

    def get_top_terms(self):
        if self.equi is not None:
            for term in self.equi.getTerms():
                yield Term([], term)

    def get_leaf_readers(self):
        if self.equi is not None:
            for term in self.equi.getInstTermOccurrences():
                direction = term.getInstTerm().getDirection()
                if direction != snl.SNLTerm.Direction.Output:
                    if term.getInstTerm().getInstance().getModel().isLeaf():
                        yield Term(
                            snl.SNLPath(term.getPath(), term.getInstTerm().getInstance()),
                            term.getInstTerm().getBitTerm(),
                        )

    def get_leaf_drivers(self):
        if self.equi is not None:
            for term in self.equi.getInstTermOccurrences():
                direction = term.getInstTerm().getDirection()
                if direction != snl.SNLTerm.Direction.Input:
                    if term.getInstTerm().getInstance().getModel().isLeaf():
                        yield Term(
                            snl.SNLPath(term.getPath(), term.getInstTerm().getInstance()),
                            term.getInstTerm().getBitTerm(),
                        )

    def get_top_readers(self):
        if self.equi is not None:
            for term in self.equi.getTerms():
                direction = term.getDirection()
                if direction != snl.SNLTerm.Direction.Input:
                    yield Term(snl.SNLPath(), term)

    def get_top_drivers(self):
        if self.equi is not None:
            for term in self.equi.getTerms():
                direction = term.getDirection()
                if direction != snl.SNLTerm.Direction.Output:
                    yield Term(snl.SNLPath(), term)


class Net:
    def __init__(self, path, net=None, net_concat=None):
        if net is not None and net_concat is not None:
            raise ValueError(
                "Only one of `net` or `net_concat` should be provided, not both."
            )
        if isinstance(path, snl.SNLPath):
            if path.size() > 0:
                self.pathIDs = path.getPathIDs()
            else:
                self.pathIDs = []
        elif isinstance(path, list):
            self.pathIDs = path.copy()
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
        path = get_snl_path_from_id_list(self.pathIDs)
        if path.size() > 0:
            return f"{path}/{net_str}"
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

    def is_const(self) -> bool:
        """Return True if the net is a constant generator."""
        if hasattr(self, "net"):
            return self.net.isConstant()
        for net in self.net_concat:
            if not net.isConstant():
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
                    yield Net(self.pathIDs, bit)
            else:
                yield self
        else:
            for net in self.net_concat:
                yield net

    def get_bit(self, index: int):
        if hasattr(self, "net"):
            if isinstance(self.net, snl.SNLBusNet):
                return Net(self.pathIDs, self.net.getBit(index))
            else:
                return None
        if 0 <= index < len(self.net_concat):
            return Net(self.pathIDs, self.net_concat[index])
        return None

    def get_inst_terms(self):
        if hasattr(self, "net_concat"):
            raise ValueError("Cannot get inst terms from a net_concat")
        for term in self.net.getInstTerms():
            path = self.pathIDs.copy()
            path.append(term.getInstance().getID())
            yield Term(path, term.getBitTerm())

    def get_design_terms(self):
        if hasattr(self, "net_concat"):
            raise ValueError("Cannot get terms from a net_concat")
        for term in self.net.getBitTerms():
            yield Term(self.pathIDs, term)

    def get_terms(self):
        for term in itertools.chain(self.get_design_terms(), self.get_inst_terms()):
            yield term


def get_snl_term_for_ids(pathIDs, termIDs):
    path = get_snl_path_from_id_list(pathIDs)
    model = None
    if len(pathIDs) == 0:
        model = snl.SNLUniverse.get().getTopDesign()
    else:
        model = path.getTailInstance().getModel()
    if termIDs[1] == -1:
        return model.getTermByID(termIDs[0])
    else:
        snlterm = model.getTermByID(termIDs[0])
        if isinstance(snlterm, snl.SNLBusTerm):
            return snlterm.getBit(termIDs[1])
        else:
            return snlterm


class Term:
    INPUT = snl.SNLTerm.Direction.Input
    OUTPUT = snl.SNLTerm.Direction.Output
    INOUT = snl.SNLTerm.Direction.InOut

    def __init__(self, path, term):
        self.termIDs = []
        if isinstance(term, snl.SNLBusTerm):
            self.termIDs = [term.getID(), -1]
        else:
            self.termIDs = [term.getID(), term.getBit()]

        if isinstance(path, snl.SNLPath):
            if path.size() > 0:
                self.pathIDs = path.getPathIDs()
            else:
                self.pathIDs = []
        elif isinstance(path, list):
            self.pathIDs = path.copy()

    def __eq__(self, other) -> bool:
        return self.pathIDs == other.pathIDs and self.termIDs == other.termIDs

    def __ne__(self, other) -> bool:
        return not self == other

    def __lt__(self, other) -> bool:
        if self.pathIDs != other.pathIDs:
            return self.pathIDs < other.pathIDs
        return self.termIDs < other.termIDs

    def __le__(self, other) -> bool:
        return self < other or self == other

    def __gt__(self, other) -> bool:
        return not self <= other

    def __ge__(self, other) -> bool:
        return not self < other

    def __hash__(self):
        termIDs = []
        snlterm = get_snl_term_for_ids(self.pathIDs, self.termIDs)
        if isinstance(snlterm, snl.SNLBusTerm):
            termIDs = [snlterm.getID(), -1]
        else:
            termIDs = [snlterm.getID(), snlterm.getBit()]
        return consistent_hash((self.pathIDs, termIDs))

    def __str__(self):
        path = get_snl_path_from_id_list(self.pathIDs)
        if path.size() == 0:
            return get_snl_term_for_ids(self.pathIDs, self.termIDs).getName()
        else:
            return f"{path}/{get_snl_term_for_ids(self.pathIDs, self.termIDs).getName()}"

    def __repr__(self) -> str:
        path = get_snl_path_from_id_list(self.pathIDs)
        return f"Term({path}, {get_snl_term_for_ids(self.pathIDs, self.termIDs).getName()})"

    def __make_unique(self):
        path = get_snl_path_from_id_list(self.pathIDs)
        if path.size() > 1:
            path = path.getHeadPath()
            snl.SNLUniquifier(path)

    def is_bus(self) -> bool:
        """Return True if the term is a bus."""
        return isinstance(get_snl_term_for_ids(self.pathIDs, self.termIDs), snl.SNLBusTerm)

    def is_bus_bit(self) -> bool:
        """Return True if the term is a bit of a bus."""
        return isinstance(get_snl_term_for_ids(self.pathIDs, self.termIDs), snl.SNLBusTermBit)

    def is_scalar(self) -> bool:
        """Return True if the term is a scalar."""
        return isinstance(get_snl_term_for_ids(self.pathIDs, self.termIDs), snl.SNLScalarTerm)

    def is_bit(self) -> bool:
        """Return True if the term is a bit."""
        return self.is_scalar() or self.is_bus_bit()

    def get_msb(self) -> int:
        """Return the most significant bit of the term if it is a bus."""
        if isinstance(get_snl_term_for_ids(self.pathIDs, self.termIDs), snl.SNLBusTerm):
            return get_snl_term_for_ids(self.pathIDs, self.termIDs).getMSB()
        return None

    def get_lsb(self) -> int:
        """Return the least significant bit of the term if it is a bus."""
        if isinstance(get_snl_term_for_ids(self.pathIDs, self.termIDs), snl.SNLBusTerm):
            return get_snl_term_for_ids(self.pathIDs, self.termIDs).getLSB()
        return None

    def get_width(self) -> int:
        """Return the width of the term. 1 if scalar."""
        return get_snl_term_for_ids(self.pathIDs, self.termIDs).getWidth()

    def get_name(self) -> str:
        """Return the name of the term."""
        return get_snl_term_for_ids(self.pathIDs, self.termIDs).getName()

    def get_direction(self) -> snl.SNLTerm.Direction:
        """Return the direction of the term."""
        snlterm = get_snl_term_for_ids(self.pathIDs, self.termIDs)
        if snlterm.getDirection() == snl.SNLTerm.Direction.Input:
            return Term.INPUT
        elif snlterm.getDirection() == snl.SNLTerm.Direction.Output:
            return Term.OUTPUT
        elif snlterm.getDirection() == snl.SNLTerm.Direction.InOut:
            return Term.INOUT

    def __get_snl_bitnet(self, bit) -> Net:
        # single bit
        path = get_snl_path_from_id_list(self.pathIDs)
        if path.size() > 0:
            instTerm = path.getTailInstance().getInstTerm(bit)
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
        if isinstance(get_snl_term_for_ids(self.pathIDs, self.termIDs), snl.SNLBusTerm):
            snl_nets = []
            for bit in get_snl_term_for_ids(self.pathIDs, self.termIDs).getBits():
                snl_net = snl_term_net_accessor(bit)
                snl_nets.append(snl_net)
            snl_bus_net = self.__get_snl_busnet(snl_nets)
            if snl_bus_net is not None:
                return Net(path, snl_bus_net)
            else:
                if all(element is not None for element in snl_nets):
                    return Net(path, net_concat=snl_nets)
        else:
            snl_net = snl_term_net_accessor(get_snl_term_for_ids(self.pathIDs, self.termIDs))
            if snl_net is not None:
                return Net(path, snl_net)
        return None

    def get_lower_net(self) -> Net:
        """Return the lower net of the term."""
        return self.__get_net(self.pathIDs, self.__get_snl_lower_bitnet)

    def get_net(self) -> Net:
        """Return the net of the term."""
        head_path = self.pathIDs.copy()
        if len(head_path) == 0:
            return None
        # path is one level up
        head_path.pop()
        return self.__get_net(head_path, self.__get_snl_bitnet)

    def get_instance(self):
        """Return the instance of the term."""
        return Instance(self.pathIDs)

    def get_flat_fanout(self):
        return self.get_equipotential().get_leaf_readers()

    def get_equipotential(self) -> Equipotential:
        return Equipotential(self)

    def is_input(self) -> bool:
        """Return True if the term is an input."""
        snlterm = get_snl_term_for_ids(self.pathIDs, self.termIDs)
        return snlterm.getDirection() == snl.SNLTerm.Direction.Input

    def is_output(self) -> bool:
        """Return True if the term is an output."""
        snlterm = get_snl_term_for_ids(self.pathIDs, self.termIDs)
        return snlterm.getDirection() == snl.SNLTerm.Direction.Output

    def get_bits(self):
        if isinstance(get_snl_term_for_ids(self.pathIDs, self.termIDs), snl.SNLBusTerm):
            for bit in get_snl_term_for_ids(self.pathIDs, self.termIDs).getBits():
                yield Term(self.pathIDs, bit)
        else:
            yield self

    def get_bit(self, index: int):
        if isinstance(get_snl_term_for_ids(self.pathIDs, self.termIDs), snl.SNLBusTerm):
            return Term(self.pathIDs, get_snl_term_for_ids(self.pathIDs, self.termIDs).getBit(index))
        return None

    def disconnect(self):
        path = get_snl_path_from_id_list(self.pathIDs)
        self.__make_unique()
        inst = path.getTailInstance()
        for bit in get_snl_term_for_ids(self.pathIDs, self.termIDs).getBits():
            iterm = inst.getInstTerm(bit)
            iterm.setNet(None)

    def connect(self, net: Net):
        if self.get_width() != net.get_width():
            raise ValueError("Width mismatch")
        if self.get_instance().is_top():
            for bterm, bnet in zip(get_snl_term_for_ids(self.pathIDs,
                                                        self.termIDs).getBits(),
                                   net.net.getBits()):
                logging.debug(f"Connecting {bterm} to {bnet}")
                bterm.setNet(bnet)
        else:
            self.__make_unique()
            path = get_snl_path_from_id_list(self.pathIDs)
            inst = path.getTailInstance()
            for bterm, bnet in zip(get_snl_term_for_ids(self.pathIDs,
                                                        self.termIDs).getBits(),
                                   net.net.getBits()):
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
    pathlist = path.getPathIDs()
    assert len(pathlist) > 0
    path = snl.SNLPath()
    instance = None
    top = snl.SNLUniverse.get().getTopDesign()
    design = top
    for id in pathlist:
        path = snl.SNLPath(path, design.getInstanceByID(id))
        instance = design.getInstanceByID(id)
        assert instance is not None
        design = instance.getModel()
    return path


class Instance:
    """Class that represents the instance and wraps some
    of the snl occurrence API.
    """

    def __init__(self, path=snl.SNLPath()):
        if isinstance(path, snl.SNLPath):
            if path.size() > 0:
                self.pathIDs = path.getPathIDs()
            else:
                self.pathIDs = []
        elif isinstance(path, list):
            self.pathIDs = path.copy()

    def __eq__(self, other) -> bool:
        return self.pathIDs == other.pathIDs

    def __str__(self):
        if self.is_top():
            top = self.__get_snl_model()
            if top is not None:
                return top.getName()
            else:
                return ""
        else:
            path = get_snl_path_from_id_list(self.pathIDs)
            return str(path)

    def __repr__(self) -> str:
        path = get_snl_path_from_id_list(self.pathIDs)
        return f"Instance({path})"

    def __hash__(self):
        return consistent_hash(self.pathIDs)

    def get_leaf_children(self):
        initial_path = get_snl_path_from_id_list(self.pathIDs)
        for inst in self.__get_snl_model().getInstances():
            if inst.getModel().isLeaf():
                yield Instance(snl.SNLPath(initial_path, inst))
            path = snl.SNLPath(initial_path, inst)
            stack = [[inst, path]]
            while stack:
                current = stack.pop()
                current_inst = current[0]
                current_path = current[1]
                for inst_child in current_inst.getModel().getInstances():
                    path_child = snl.SNLPath(current_path, inst_child)
                    if inst_child.getModel().isLeaf():
                        yield Instance(path_child)
                    stack.append([inst_child, path_child])

    def is_top(self) -> bool:
        """Return True if this is the top design."""
        return len(self.pathIDs) == 0

    def is_assign(self) -> bool:
        return self.__get_snl_model().isAssign()

    def is_blackbox(self) -> bool:
        """Return True if this is a blackbox."""
        return self.__get_snl_model().isBlackBox()

    def is_leaf(self) -> bool:
        """Return True if this is a leaf."""
        return self.__get_snl_model().isLeaf()

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
        instance = get_snl_instance_from_id_list(self.pathIDs)
        return instance.getModel()

    def __find_snl_model(self, name: str) -> snl.SNLDesign:
        u = snl.SNLUniverse.get()
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
        path = self.pathIDs.copy()
        path.append(childInst.getID())
        return Instance(path)

    def get_child_instances(self):
        for inst in self.__get_snl_model().getInstances():
            path = self.pathIDs.copy()
            path.append(inst.getID())
            yield Instance(path)

    def get_number_of_child_instances(self) -> int:
        return sum(1 for _ in self.__get_snl_model().getInstances())

    # def get_flat_primitive_instances(self):
    #    FIXME: concat first local path with the path of the instance
    #    model = self.__get_snl_model()
    #    for inst in model.getInstances():
    #        path = snl.SNLPath(inst)
    #        stack = [[inst, path]]
    #        while stack:
    #            current = stack.pop()
    #            current_inst = current[0]
    #            current_path = current[1]
    #            for inst_child in current_inst.getModel().getInstances():
    #                path_child = snl.SNLPath(current_path, inst_child)
    #                if inst_child.getModel().isPrimitive():
    #                    yield Instance(path_child)
    #                stack.append([inst_child, path_child])

    def get_nets(self):
        for net in self.__get_snl_model().getNets():
            yield Net(self.pathIDs, net)

    def get_flat_nets(self):
        for net in self.__get_snl_model().getNets():
            if isinstance(net, snl.SNLBusNet):
                for bit in net.getBits():
                    yield Net(self.pathIDs, bit)
            else:
                yield Net(self.pathIDs, net)

    def get_net(self, name: str) -> Net:
        net = self.__get_snl_model().getNet(name)
        if net is not None:
            return Net(self.pathIDs, net)
        return None

    def is_primitive(self) -> bool:
        """Return True if this is a primitive."""
        return self.__get_snl_model().isPrimitive()

    def get_terms(self):
        for term in self.__get_snl_model().getTerms():
            yield Term(self.pathIDs, term)

    def get_flat_terms(self):
        for term in self.__get_snl_model().getBitTerms():
            yield Term(self.pathIDs, term)

    def get_term(self, name: str) -> Term:
        term = self.__get_snl_model().getTerm(name)
        if term is not None:
            return Term(self.pathIDs, self.__get_snl_model().getTerm(name))
        return None

    def get_input_terms(self):
        for term in self.__get_snl_model().getTerms():
            if term.getDirection() != snl.SNLTerm.Direction.Output:
                yield Term(self.pathIDs, term)

    def get_flat_input_terms(self):
        for term in self.__get_snl_model().getTerms():
            if term.getDirection() != snl.SNLTerm.Direction.Output:
                if isinstance(term, snl.SNLBusTerm):
                    for bit in term.getBits():
                        yield Term(self.pathIDs, bit)
                else:
                    yield Term(self.pathIDs, term)

    def get_output_terms(self):
        for term in self.__get_snl_model().getTerms():
            if term.getDirection() != snl.SNLTerm.Direction.Input:
                yield Term(self.pathIDs, term)

    def get_flat_output_terms(self):
        for term in self.__get_snl_model().getTerms():
            if term.getDirection() != snl.SNLTerm.Direction.Input:
                if isinstance(term, snl.SNLBusTerm):
                    for bit in term.getBits():
                        yield Term(self.pathIDs, bit)
                else:
                    yield Term(self.pathIDs, term)

    def delete_instance(self, name: str):
        init_path = get_snl_path_from_id_list(self.pathIDs)
        if name == "":
            raise ValueError(
                "Cannot delete instance with empty name. Try delete_instance_by_id instead."
            )
        path = snl.SNLPath(init_path, self.__get_snl_model().getInstance(name))
        snl.SNLUniquifier(path)
        if init_path.size() > 0:
            # Delete the last instance in uniq_path
            self.__get_snl_model().getInstance(name).destroy()

    def delete_instance_by_id(self, id: str):
        init_path = get_snl_path_from_id_list(self.pathIDs)
        path = snl.SNLPath(init_path, self.__get_snl_model().getInstanceByID(id))
        snl.SNLUniquifier(path)
        # Delete the last instance in uniq_path
        self.__get_snl_model().getInstanceByID(id).destroy()

    def get_design(self):
        path = self.pathIDs.copy()
        if len(self.pathIDs) == 1:
            return get_top()
        path.pop()
        return Instance(path)

    def delete(self):
        path = get_snl_path_from_id_list(self.pathIDs)
        snl.SNLUniquifier(path)
        self.get_design().delete_instance_by_id(path.getTailInstance().getID())

    def get_name(self) -> str:
        path = get_snl_path_from_id_list(self.pathIDs)
        """Return the name of the instance or name of the top is this is the top."""
        if self.is_top():
            return self.get_model_name()
        else:
            return path.getTailInstance().getName()

    def get_model_name(self) -> str:
        """Return the name of the model of the instance or name of the top is this is the top."""
        return self.__get_snl_model().getName()

    def get_model_id(self) -> tuple[int, int, int]:
        model = self.__get_snl_model()
        return model.getDB().getID(), model.getLibrary().getID(), model.getID()

    def create_child_instance(self, model: str, name: str):
        path = get_snl_path_from_id_list(self.pathIDs)
        if path.size() > 0:
            snl.SNLUniquifier(path)
            path = get_snl_path_from_id_list(self.pathIDs)
        design = self.__get_snl_model()
        new_instance_model = self.__find_snl_model(model)
        if new_instance_model is None:
            raise ValueError(
                f"Cannot create instance {name} in {self}: model {model} cannot be found"
            )
        newSNLInstance = snl.SNLInstance.create(design, new_instance_model, name)
        path = snl.SNLPath(path, newSNLInstance)
        return Instance(path)

    def create_term(self, name: str, direction: snl.SNLTerm.Direction) -> Term:
        path = get_snl_path_from_id_list(self.pathIDs)
        if path.size() > 0:
            snl.SNLUniquifier(path)
            path = get_snl_path_from_id_list(self.pathIDs)
        design = self.__get_snl_model()
        newSNLTerm = snl.SNLScalarTerm.create(design, direction, name)
        return Term(path, newSNLTerm)

    def create_output_term(self, name: str) -> Term:
        return self.create_term(name, snl.SNLTerm.Direction.Output)

    def create_input_term(self, name: str) -> Term:
        return self.create_term(name, snl.SNLTerm.Direction.Input)

    def create_inout_term(self, name: str) -> Term:
        return self.create_term(name, snl.SNLTerm.Direction.InOut)

    def create_bus_term(self, name: str, msb: int, lsb: int, direction) -> Term:
        path = get_snl_path_from_id_list(self.pathIDs)
        if path.size() > 0:
            snl.SNLUniquifier(path)
            path = get_snl_path_from_id_list(self.pathIDs)
        design = self.__get_snl_model()
        newSNLTerm = snl.SNLBusTerm.create(design, direction, msb, lsb, name)
        return Term(path, newSNLTerm)

    def create_inout_bus_term(self, name: str, msb: int, lsb: int) -> Term:
        return self.create_bus_term(name, msb, lsb, snl.SNLTerm.Direction.InOut)

    def create_output_bus_term(self, name: str, msb: int, lsb: int) -> Term:
        return self.create_bus_term(name, msb, lsb, snl.SNLTerm.Direction.Output)

    def create_input_bus_term(self, name: str, msb: int, lsb: int) -> Term:
        return self.create_bus_term(name, msb, lsb, snl.SNLTerm.Direction.Input)

    def create_net(self, name: str) -> Net:
        path = get_snl_path_from_id_list(self.pathIDs)
        if path.size() > 0:
            snl.SNLUniquifier(path)
            path = get_snl_path_from_id_list(self.pathIDs)
        model = self.__get_snl_model()
        newSNLNet = snl.SNLScalarNet.create(model, name)
        return Net(path, newSNLNet)

    def create_bus_net(self, name: str, msb: int, lsb: int) -> Net:
        path = get_snl_path_from_id_list(self.pathIDs)
        if path.size() > 0:
            snl.SNLUniquifier(path)
            path = get_snl_path_from_id_list(self.pathIDs)
        model = self.__get_snl_model()
        newSNLNet = snl.SNLBusNet.create(model, msb, lsb, name)
        return Net(path, newSNLNet)

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
    start_time = time.time()
    logging.info(f"Loading verilog: {', '.join(files)}")
    get_top_db().loadVerilog(files)
    execution_time = time.time() - start_time
    logging.info(f"Loading done in {execution_time:.2f} seconds")
    return get_top()


def load_liberty(files: list):
    logging.info(f"Loading liberty: {', '.join(files)}")
    get_top_db().loadLibertyPrimitives(files)


def load_primitives(name: str):
    if name == "xilinx":
        logging.info("Loading xilinx primitives")
        from najaeda.primitives import xilinx

        xilinx.load(get_top_db())
    else:
        raise ValueError(f"Unknown primitives library: {name}")


def get_primitives_library() -> snl.SNLLibrary:
    lib = get_top_db().getLibrary("PRIMS")
    if lib is None:
        lib = snl.SNLLibrary.createPrimitives(get_top_db(), "PRIMS")
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
