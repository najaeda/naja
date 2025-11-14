# SPDX-FileCopyrightText: 2024 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import time
import logging
import hashlib
import struct
import sys
import os
from enum import Enum
from typing import Union, List, Iterator
from dataclasses import dataclass

from najaeda import naja


def get_none_existent():
    return sys.maxsize


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
            return struct.pack("!q", value)
        else:
            raise TypeError(f"Unsupported type: {type(value)}")

    def hash_object(o):
        if isinstance(o, (list, tuple)):
            return b"".join(hash_object(i) for i in o)
        else:
            return hash_value(o)

    serialized_obj = default_serializer(obj)
    obj_bytes = hash_object(serialized_obj)
    return int(hashlib.sha256(obj_bytes).hexdigest(), 16)


def get_snl_instance_from_id_list(id_list: list) -> naja.SNLInstance:
    design = naja.NLUniverse.get().getTopDesign()
    # instance = None
    # for id in id_list:
    #     instance = design.getInstanceByID(id)
    #     assert instance is not None
    #     design = instance.getModel()
    # return instance
    return design.getInstanceByIDList(id_list)


def get_snl_path_from_id_list(id_list: list) -> naja.SNLPath:
    top = naja.NLUniverse.get().getTopDesign()
    design = top
    path = naja.SNLPath()
    for id in id_list:
        instance = design.getInstanceByID(id)
        assert instance is not None
        path = naja.SNLPath(path, instance)
        assert path.getTailInstance() is not None
        design = instance.getModel()
    if len(id_list) > 0:
        assert path.getTailInstance() is not None
    return path


class Attribute:
    def __init__(self, snlAttribute):
        self.snlAttribute = snlAttribute

    def __str__(self):
        return str(self.snlAttribute)

    def get_name(self):
        """
        :return: the name of the attribute.
        :rtype: str
        """
        return self.snlAttribute.getName()

    def has_value(self):
        """
        :return: True if the attribute has a value.
        :rtype: bool
        """
        return self.snlAttribute.hasValue()

    def get_value(self):
        """
        :return: the value of the attribute.
        :rtype: str
        """
        return self.snlAttribute.getValue()


class Equipotential:
    """Class that represents the term and wraps
    some of the snl occurrence API.
    """

    def __init__(self, term):
        path = get_snl_path_from_id_list(term.pathIDs)
        snl_term = get_snl_term_for_ids_with_path(path, term.termIDs)
        inst_term = None
        if isinstance(snl_term, naja.SNLBusTerm):
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
                path = naja.SNLPath(path, get_snl_instance_from_id_list(inst_term.pathIDs))
                snl_term = get_snl_term_for_ids(inst_term.pathIDs, inst_term.termIDs)
        else:
            inst_term = term
        ito = naja.SNLOccurrence(
            path.getHeadPath(), path.getTailInstance().getInstTerm(snl_term)
        )
        self.equi = naja.SNLEquipotential(ito)

    def __eq__(self, value):
        return self.equi == value.equi

    def dump_dot(self, path: str):
        """Dump the dot file of this equipotential."""
        self.equi.dumpDotFile(path)

    def get_inst_terms(self):
        """Iterate over the instance terminals of this equipotential.

        :return: an iterator over the instance terminals of this equipotential.
        :rtype: Iterator[Term]
        """
        if self.equi is not None:
            for term in self.equi.getInstTermOccurrences():
                path = term.getPath().getInstanceIDs()
                path.append(term.getInstTerm().getInstance().getID())
                yield Term(path,
                    term.getInstTerm().getBitTerm())

    def get_top_terms(self):
        """Iterate over the top terminals of this equipotential.

        :return: an iterator over the top terminals of this equipotential.
        :rtype: Iterator[Term]
        """
        if self.equi is not None:
            for term in self.equi.getTerms():
                yield Term([], term)

    def get_leaf_readers(self, filter=None):
        if self.equi is not None:
            for term in self.equi.getInstTermOccurrences():
                direction = term.getInstTerm().getDirection()
                if (
                    direction != naja.SNLTerm.Direction.Output and
                    term.getInstTerm().getInstance().getModel().isLeaf() and
                    (filter is None or filter(term))
                ):
                    path = term.getPath().getInstanceIDs()
                    path.append(term.getInstTerm().getInstance().getID())
                    yield Term(path, term.getInstTerm().getBitTerm())

    def get_leaf_drivers(self, filter=None):
        if self.equi is not None:
            for term in self.equi.getInstTermOccurrences():
                direction = term.getInstTerm().getDirection()
                if (
                    direction != naja.SNLTerm.Direction.Input and
                    term.getInstTerm().getInstance().getModel().isLeaf() and
                    (filter is None or filter(term))
                ):
                    path = term.getPath().getInstanceIDs()
                    path.append(term.getInstTerm().getInstance().getID())
                    yield Term(path, term.getInstTerm().getBitTerm())

    def get_top_readers(self):
        if self.equi is not None:
            for term in self.equi.getTerms():
                direction = term.getDirection()
                if direction != naja.SNLTerm.Direction.Input:
                    yield Term([], term)

    def get_top_drivers(self):
        if self.equi is not None:
            for term in self.equi.getTerms():
                direction = term.getDirection()
                if direction != naja.SNLTerm.Direction.Output:
                    yield Term([], term)


class Net:
    class Type(Enum):
        STANDARD = naja.SNLNet.Type.Standard
        ASSIGN0 = naja.SNLNet.Type.Assign0
        ASSIGN1 = naja.SNLNet.Type.Assign1
        SUPPLY0 = naja.SNLNet.Type.Supply0
        SUPPLY1 = naja.SNLNet.Type.Supply1

    def __init__(self, path, net=None, net_concat=None):
        if net is not None and net_concat is not None:
            raise ValueError(
                "Only one of `net` or `net_concat` should be provided, not both."
            )
        if isinstance(path, naja.SNLPath):
            if path.size() > 0:
                self.pathIDs = path.getInstanceIDs()
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
        if self.is_concat():
            net_str = "{" + ",".join(map(str, self.net_concat)) + "}"
            return net_str
        net_str = str(self.net)
        path = get_snl_path_from_id_list(self.pathIDs)
        if path.size() > 0:
            return f"{path}/{net_str}"
        else:
            return net_str

    def delete(self):
        if hasattr(self, "net"):
            self.net.destroy()
        else:
            for net in self.net_concat:
                net.destroy()

    def get_name(self) -> str:
        """
        :return: the name of this Net.
        :rtype: str
        """
        if hasattr(self, "net"):
            return self.net.getName()
        return "{" + ",".join(map(str, self.net_concat)) + "}"

    def set_name(self, name: str):
        """
        :param str name: the name to set for this Net.
        """
        if self.is_concat():
            raise ValueError(
                f"set_name of {self}: cannot set name for a concatenated net. "
                "Use the individual nets instead."
            )
        elif self.is_bus_bit():
            raise ValueError(
                f"set_name of {self}: cannot set name for a bus bit. "
                "Use the bus net instead."
            )
        else:
            # We need to uniquify until parent instance if parent instance is not top.
            path = get_snl_path_from_id_list(self.pathIDs)
            if path.size():
                naja.SNLUniquifier(path.getHeadPath())
            self.net.setName(name)

    def get_msb(self) -> int:
        """
        :return: the most significant bit of the net if it is a bus.
        :rtype: int
        """
        if hasattr(self, "net") and isinstance(self.net, naja.SNLBusNet):
            return self.net.getMSB()
        return None

    def get_lsb(self) -> int:
        """
        :return: the least significant bit of the net if it is a bus.
        :rtype: int
        """
        if hasattr(self, "net") and isinstance(self.net, naja.SNLBusNet):
            return self.net.getLSB()
        return None

    def is_bus(self) -> bool:
        """
        :return: True if the net is a bus.
        :rtype: bool
        """
        return hasattr(self, "net") and isinstance(self.net, naja.SNLBusNet)

    def is_bus_bit(self) -> bool:
        """
        :return: True if the net is a bit of a bus.
        :rtype: bool
        """
        return hasattr(self, "net") and isinstance(self.net, naja.SNLBusNetBit)

    def is_scalar(self) -> bool:
        """
        :return: True if the net is a scalar.
        :rtype: bool
        """
        return hasattr(self, "net") and isinstance(self.net, naja.SNLScalarNet)

    def is_bit(self) -> bool:
        """
        :return: True if the net is a bit.
        :rtype: bool
        """
        return self.is_scalar() or self.is_bus_bit()

    def is_concat(self) -> bool:
        """
        :return: True if the net is a concatenation.
        :rtype: bool
        """
        return hasattr(self, "net_concat")

    def is_const(self) -> bool:
        """
        :return: True if the net is a constant generator.
        :rtype: bool
        """
        if hasattr(self, "net"):
            return self.net.isConstant()
        else:
            return all(net.isConstant() for net in self.net_concat)

    def set_type(self, net_type: Type):
        """
        :param Type net_type: the type of the net.
        """
        if hasattr(self, "net"):
            self.net.setType(net_type.value)
        else:
            for net in self.net_concat:
                net.setType(net_type.value)

    def get_width(self) -> int:
        """
        :return: the width of the net.
        :rtype: int
        """
        if hasattr(self, "net"):
            return self.net.getWidth()
        return sum(1 for _ in self.net_concat)

    def get_bits(self):
        """Iterate over the bits of this Net.
        The iterator will return itself if the Net is scalar.
        :return: an iterator over the bits of this Net.
        :rtype: Iterator[Net]
        """
        if hasattr(self, "net"):
            if isinstance(self.net, naja.SNLBusNet):
                for bit in self.net.getBits():
                    yield Net(self.pathIDs, bit)
            else:
                yield self
        else:
            for net in self.net_concat:
                yield Net(self.pathIDs, net)

    def get_bit(self, index: int):
        """
        :param int index: the index of the bit to get.
        :return: the Net bit at the given index or None if it does not exist.
        :rtype: Net
        """
        if hasattr(self, "net"):
            if isinstance(self.net, naja.SNLBusNet):
                return Net(self.pathIDs, self.net.getBit(index))
            else:
                return None
        if 0 <= index < len(self.net_concat):
            return Net(self.pathIDs, self.net_concat[index])
        return None

    def get_inst_terms(self):
        """
        :return: an iterator over the instance terminals of the net.
        :rtype: Iterator[Term]
        """
        if hasattr(self, "net_concat"):
            raise ValueError("Cannot get inst terms from a net_concat")
        path = self.pathIDs.copy()
        for term in self.net.getInstTerms():
            path.append(term.getInstance().getID())
            yield Term(path, term.getBitTerm())
            path.pop()

    def count_inst_terms(self) -> int:
        """
        Count the instance terminals of this net.

        :return: the number of instance terminals of this net.
        :rtype: int
        """
        return sum(1 for _ in self.get_inst_terms())

    def get_design_terms(self):
        """Return an iterator over the design terminals of the net.
        This includes only the terminals that are part of the current design.
        The iterator will yield Term objects bit per bit.

        :return: an iterator over the design terminals of the net.
        :rtype: Iterator[Term]
        """
        if hasattr(self, "net_concat"):
            raise ValueError("Cannot get terms from a net_concat")
        for term in self.net.getBitTerms():
            yield Term(self.pathIDs, term)

    def count_design_terms(self) -> int:
        """Count the design terminals of this net.

        :return: the number of design terminals of this net.
        :rtype: int
        """
        return sum(1 for _ in self.get_design_terms())

    def get_terms(self):
        """Return an iterator over the terminals of the net.
        This includes both design and instance terminals.

        :return: an iterator over the terminals of the net.
        :rtype: Iterator[Term]
        """
        yield from self.get_design_terms()
        yield from self.get_inst_terms()

    def get_attributes(self) -> Iterator[Attribute]:
        """Iterate over the attributes of this Net.

        :return: the attributes of this Net.
        :rtype: Iterator[Attribute]
        """
        if hasattr(self, "net"):
            snlnet = self.net
            for attribute in snlnet.getAttributes():
                yield Attribute(attribute)

    def count_attributes(self) -> int:
        """Count the attributes of this Net.

        :return: the number of attributes of this Net.
        :rtype: int
        """
        return sum(1 for _ in self.get_attributes())


def get_snl_term_for_ids(pathIDs, termIDs):
    path = get_snl_path_from_id_list(pathIDs)
    model = None
    if len(pathIDs) == 0:
        model = naja.NLUniverse.get().getTopDesign()
    else:
        model = path.getTailInstance().getModel()
    if termIDs[1] == get_none_existent():
        return model.getTermByID(termIDs[0])
    else:
        snlterm = model.getTermByID(termIDs[0])
        if isinstance(snlterm, naja.SNLBusTerm):
            return snlterm.getBusTermBit(termIDs[1])
        else:
            return snlterm


def get_snl_term_for_ids_with_path(path, termIDs):
    model = None
    if path.size() == 0:
        model = naja.NLUniverse.get().getTopDesign()
    else:
        model = path.getTailInstance().getModel()
    if termIDs[1] == get_none_existent():
        return model.getTermByID(termIDs[0])
    else:
        snlterm = model.getTermByID(termIDs[0])
        if isinstance(snlterm, naja.SNLBusTerm):
            return snlterm.getBusTermBit(termIDs[1])
        else:
            return snlterm


class Term:
    class Direction(Enum):
        """Enum for the direction of a term."""
        INPUT = naja.SNLTerm.Direction.Input
        OUTPUT = naja.SNLTerm.Direction.Output
        INOUT = naja.SNLTerm.Direction.InOut

    def __init__(self, path, term):
        self.termIDs = [term.getID(), term.getBit()]
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
        if isinstance(snlterm, naja.SNLBusTerm):
            termIDs = [snlterm.getID(), -1]
        else:
            termIDs = [snlterm.getID(), snlterm.getBit()]
        return consistent_hash((self.pathIDs, termIDs))

    def __str__(self):
        term_str = ""
        path = get_snl_path_from_id_list(self.pathIDs)
        snl_term = get_snl_term_for_ids(self.pathIDs, self.termIDs)
        snl_term_str = snl_term.getName()
        if snl_term.isUnnamed():
            snl_term_str = "<unnamed>"
        if path.size() == 0:
            term_str = snl_term_str
        else:
            term_str = f"{path}/{snl_term_str}"
        if self.is_bus():
            term_str += f"[{self.get_msb()}:{self.get_lsb()}]"
        bit = self.get_bit_number()
        if bit is not None:
            term_str += f"[{bit}]"
        return term_str

    def __repr__(self) -> str:
        path = get_snl_path_from_id_list(self.pathIDs)
        return f"Term({path}, {get_snl_term_for_ids(self.pathIDs, self.termIDs).getName()})"

    def __make_unique(self):
        path = get_snl_path_from_id_list(self.pathIDs)
        if path.size() > 1:
            path = path.getHeadPath()
            naja.SNLUniquifier(path)

    def is_bus(self) -> bool:
        """
        :return: True if the term is a bus.
        :rtype: bool
        """
        return isinstance(
            get_snl_term_for_ids(self.pathIDs, self.termIDs), naja.SNLBusTerm
        )

    def is_bus_bit(self) -> bool:
        """
        :return: True if the term is a bit of a bus.
        :rtype: bool
        """
        return isinstance(
            get_snl_term_for_ids(self.pathIDs, self.termIDs), naja.SNLBusTermBit
        )

    def is_scalar(self) -> bool:
        """
        :return: True if the term is a scalar.
        :rtype: bool
        """
        return isinstance(
            get_snl_term_for_ids(self.pathIDs, self.termIDs), naja.SNLScalarTerm
        )

    def is_bit(self) -> bool:
        """
        :return: True if the term is a bit.
        :rtype: bool
        """
        return self.is_scalar() or self.is_bus_bit()

    def is_sequential(self) -> bool:
        """
        :return: True if the term is a sequential term.
        :rtype: bool
        """
        for term in self.get_instance().get_bit_terms():
            clockRelatedInputs = term.get_clock_related_inputs()
            if self in clockRelatedInputs:
                return True
            clockRelatedOutputs = term.get_clock_related_outputs()
            if self in clockRelatedOutputs:
                return True
            if (len(clockRelatedInputs) > 0 or len(clockRelatedOutputs) > 0) and (term == self):
                return True
        return False

    def get_bit_number(self):
        """
        :return: the bit index of the term if it is a bit.
        :rtype: int or None
        """
        if isinstance(
            get_snl_term_for_ids(self.pathIDs, self.termIDs), naja.SNLBusTermBit
        ):
            return get_snl_term_for_ids(self.pathIDs, self.termIDs).getBit()
        return None

    def get_msb(self) -> int:
        """
        :return: the most significant bit of the term if it is a bus.
        :rtype: int or None
        """
        if isinstance(get_snl_term_for_ids(self.pathIDs, self.termIDs), naja.SNLBusTerm):
            return get_snl_term_for_ids(self.pathIDs, self.termIDs).getMSB()
        return None

    def get_lsb(self) -> int:
        """
        :return: the least significant bit of the term if it is a bus.
        :rtype: int or None
        """
        if isinstance(get_snl_term_for_ids(self.pathIDs, self.termIDs), naja.SNLBusTerm):
            return get_snl_term_for_ids(self.pathIDs, self.termIDs).getLSB()
        return None

    def get_width(self) -> int:
        """
        :return: the width of the term. 1 if scalar.
        :rtype: int
        """
        return get_snl_term_for_ids(self.pathIDs, self.termIDs).getWidth()

    def get_name(self) -> str:
        """
        :return: the name of the term.
        :rtype: str
        """
        return get_snl_term_for_ids(self.pathIDs, self.termIDs).getName()

    def is_unnamed(self) -> bool:
        """
        :return: True if the term is unnamed.
        :rtype: bool
        """
        return get_snl_term_for_ids(self.pathIDs, self.termIDs).isUnnamed()

    def get_direction(self) -> Direction:
        """
        :return: the direction of the term.
        :rtype: Term.Direction
        """
        snlterm = get_snl_term_for_ids(self.pathIDs, self.termIDs)
        if snlterm.getDirection() == naja.SNLTerm.Direction.Input:
            return Term.Direction.INPUT
        elif snlterm.getDirection() == naja.SNLTerm.Direction.Output:
            return Term.Direction.OUTPUT
        elif snlterm.getDirection() == naja.SNLTerm.Direction.InOut:
            return Term.Direction.INOUT

    def get_attributes(self) -> Iterator[Attribute]:
        """Iterate over the attributes of this Term.

        :return: the attributes of this Term.
        :rtype: Iterator[Attribute]
        """
        snlterm = get_snl_term_for_ids(self.pathIDs, self.termIDs)
        for attribute in snlterm.getAttributes():
            yield Attribute(attribute)

    def count_attributes(self) -> int:
        """Count the attributes of this Term.

        :return: the number of attributes of this Term.
        :rtype: int
        """
        return sum(1 for _ in self.get_attributes())

    def get_combinatorial_inputs(self):
        """Get all combinatorial input terms of this instance.

        :return: a list of combinatorial input terms.
        :rtype: List[Term]
        """
        terms = self.__get_snl_model().getCombinatorialInputs(
            get_snl_term_for_ids(self.pathIDs, self.termIDs))
        # Convert SNL terms to Term objects
        return [Term(self.pathIDs, term) for term in terms]

    def get_combinatorial_outputs(self):
        """Get all combinatorial output terms of this instance.

        :return: a list of combinatorial output terms.
        :rtype: List[Term]
        """
        terms = self.__get_snl_model().getCombinatorialOutputs(
            get_snl_term_for_ids(self.pathIDs, self.termIDs))
        # Convert SNL terms to Term objects
        return [Term(self.pathIDs, term) for term in terms]

    def get_clock_related_inputs(self):
        """Get all input terms that are related to the given clock term.

        :param clock_term: the clock term to check for related inputs.
        :return: a list of input terms that are related to the clock term.
        :rtype: List[Term]
        """
        terms = self.__get_snl_model().getClockRelatedInputs(
            get_snl_term_for_ids(self.pathIDs, self.termIDs))
        # Convert SNL terms to Term objects
        return [Term(self.pathIDs, term) for term in terms]

    def get_clock_related_outputs(self):
        """Get all output terms that are related to the given clock term.

        :param clock_term: the clock term to check for related outputs.
        :return: a list of output terms that are related to the clock term.
        :rtype: List[Term]
        """
        terms = self.__get_snl_model().getClockRelatedOutputs(
            get_snl_term_for_ids(self.pathIDs, self.termIDs))
        # Convert SNL terms to Term objects
        return [Term(self.pathIDs, term) for term in terms]

    def __get_snl_model(self):
        snlterm = get_snl_term_for_ids(self.pathIDs, self.termIDs)
        return snlterm.getDesign()

    def __get_snl_bitnet(self, bit) -> Net:
        # single bit
        path = get_snl_path_from_id_list(self.pathIDs)
        if path.size():
            instTerm = path.getTailInstance().getInstTerm(bit)
            return instTerm.getNet()
        else:
            return bit.getNet()

    def __get_snl_lower_bitnet(self, bit) -> Net:
        return bit.getNet()

    def __get_snl_busnet(self, snl_nets) -> naja.SNLBusNet:
        # iterate on all elements of the list and check if
        # a full SNLBusNet can be reconstructed
        snl_bus_net = None
        for i in range(len(snl_nets)):
            snl_net = snl_nets[i]
            if not isinstance(snl_net, naja.SNLBusNetBit):
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
        if isinstance(get_snl_term_for_ids(self.pathIDs, self.termIDs), naja.SNLBusTerm):
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
            snl_net = snl_term_net_accessor(
                get_snl_term_for_ids(self.pathIDs, self.termIDs)
            )
            if snl_net is not None:
                return Net(path, snl_net)
        return None

    def get_lower_net(self) -> Net:
        """
        :return: the lower net of the term.
        :rtype: Net
        """
        return self.__get_net(self.pathIDs, self.__get_snl_lower_bitnet)

    def get_upper_net(self) -> Net:
        """
        :return: the upper net of the term.
        :rtype: Net
        :remark: If the term is a top level term, it will return None.
        """
        head_path = self.pathIDs.copy()
        if len(head_path) == 0:
            return None
        # path is one level up
        head_path.pop()
        return self.__get_net(head_path, self.__get_snl_bitnet)

    def get_instance(self):
        """
        :return: the instance of this Term.
        :rtype: Instance
        """
        return Instance(self.pathIDs)

    def get_flat_fanout(self, filter=None):
        return self.get_equipotential().get_leaf_readers(filter=filter)

    def count_flat_fanout(self, filter=None):
        return sum(1 for _ in self.get_flat_fanout(filter=filter))

    def get_equipotential(self) -> Equipotential:
        """
        :return: the Equipotential of this Term.
        :rtype: Equipotential
        """
        return Equipotential(self)

    def is_input(self) -> bool:
        """
        :return: True if the term is an input.
        :rtype: bool
        """
        snlterm = get_snl_term_for_ids(self.pathIDs, self.termIDs)
        return snlterm.getDirection() == naja.SNLTerm.Direction.Input

    def is_output(self) -> bool:
        """
        :return: True if the term is an output.
        :rtype: bool
        """
        snlterm = get_snl_term_for_ids(self.pathIDs, self.termIDs)
        return snlterm.getDirection() == naja.SNLTerm.Direction.Output

    def get_bits(self):
        """
        :return: an iterator over the bits of the term.
            If the term is scalar, it will return an iterator over itself.
        :rtype: Iterator[Term]
        """
        if isinstance(get_snl_term_for_ids(self.pathIDs, self.termIDs), naja.SNLBusTerm):
            for bit in get_snl_term_for_ids(self.pathIDs, self.termIDs).getBits():
                yield Term(self.pathIDs, bit)
        else:
            yield self

    def get_bit(self, index: int):
        """
        :param int index: the index of the bit to get.
        :return: the Term bit at the given index or None if it does not exist.
        :rtype: Term or None
        """
        if isinstance(get_snl_term_for_ids(self.pathIDs, self.termIDs), naja.SNLBusTerm):
            return Term(
                self.pathIDs,
                get_snl_term_for_ids(self.pathIDs, self.termIDs).getBusTermBit(index),
            )
        return None

    def disconnect_upper_net(self):
        """Disconnect this term from its upper net."""
        if self.get_instance().is_top():
            raise ValueError("Cannot disconnect the upper net of a top level term")
        path = get_snl_path_from_id_list(self.pathIDs)
        self.__make_unique()
        inst = path.getTailInstance()
        for bit in get_snl_term_for_ids(self.pathIDs, self.termIDs).getBits():
            iterm = inst.getInstTerm(bit)
            iterm.setNet(None)

    def disconnect_lower_net(self):
        """Disconnect this term from its lower net."""
        self.__make_unique()
        for bit in get_snl_term_for_ids(self.pathIDs, self.termIDs).getBits():
            bit.setNet(None)

    def connect_upper_net(self, net: Net):
        """Connect this term to the given upper Net.

        :param Net net: the upper Net to connect to.
        """
        if self.get_instance().is_top():
            raise ValueError("Cannot connect the upper net of a top level term")
        if self.get_width() != net.get_width():
            raise ValueError("Width mismatch")
        self.__make_unique()
        path = get_snl_path_from_id_list(self.pathIDs)
        inst = path.getTailInstance()
        for bterm, bnet in zip(
            get_snl_term_for_ids(self.pathIDs, self.termIDs).getBits(),
            net.net.getBits(),
        ):
            iterm = inst.getInstTerm(bterm)
            iterm.setNet(bnet)

    def connect_lower_net(self, net: Net):
        """Connect this term to the given lower Net.

        :param Net net: the lower Net to connect to.
        """
        if self.get_width() != net.get_width():
            raise ValueError("Width mismatch")
        self.__make_unique()
        for bterm, bnet in zip(
            get_snl_term_for_ids(self.pathIDs, self.termIDs).getBits(),
            net.net.getBits(),
        ):
            bterm.setNet(bnet)

    def get_truth_table(self):
        # check the index of the output
        if not self.is_output():
            raise ValueError("Truth table can only be computed for output terms")
        # get the instance
        if self.is_bus():
            raise ValueError("Truth table cannot be computed for bus terms")
        inst = self.get_instance()
        # get the model
        model = inst._Instance__get_snl_model()
        for term in model.getTerms():
            if term.getDirection() == naja.SNLTerm.Direction.Output:
                if term.getName() == self.get_name():
                    # get the truth table
                    tt = model.getTruthTableByOutputID(term.getID())
                    if tt is not None:
                        return model.getTruthTableByOutputID(term.getID())
        return None


def get_instance_by_path(names: list):
    return get_top().get_child_instance(names)


class Instance:
    """Class that represents an instance in the design hierarchy.
    """

    def __init__(self, path=naja.SNLPath()):
        self.inst = None
        self.revisionCount = 0
        self.SNLID = [0] * 6
        if isinstance(path, naja.SNLPath):
            if path.size():
                self.pathIDs = path.getInstanceIDs()
                self.revisionCount = path.getTailInstance().getModel().getRevisionCount()
                self.inst = path.getTailInstance()
            else:
                self.pathIDs = []
        elif isinstance(path, list):
            self.pathIDs = path.copy()
            if path:
                self.inst = get_snl_instance_from_id_list(path)
                self.revisionCount = self.inst.getModel().getRevisionCount()
        if self.inst:
            self.SNLID = self.inst.getModel().getNLID()

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
        """Iterate over the leaf children of this Instance.
        Equivalent to the underlying leaves of the instanciation tree.

        :return: an iterator over the leaf children Instance of this Instance.
        :rtype: Iterator[Instance]
        """
        initial_path = get_snl_path_from_id_list(self.pathIDs)
        for inst in self.__get_snl_model().getInstances():
            if inst.getModel().isLeaf():
                yield Instance(naja.SNLPath(initial_path, inst))
            path = naja.SNLPath(initial_path, inst)
            stack = [[inst, path]]
            while stack:
                current = stack.pop()
                current_inst = current[0]
                current_path = current[1]
                for inst_child in current_inst.getModel().getInstances():
                    path_child = naja.SNLPath(current_path, inst_child)
                    if inst_child.getModel().isLeaf():
                        yield Instance(path_child)
                    stack.append([inst_child, path_child])

    def is_top(self) -> bool:
        """
        :return: True if this is the top design.
        :rtype: bool
        """
        return not self.pathIDs

    def is_assign(self) -> bool:
        """(assign a=b) will create an instance of assign connecting
        the wire a to the output of the assign and b to the input.

        :return: True if this is an assign (represented by an unnamed assign instance).
        :rtype: bool
        """
        return self.__get_snl_model().isAssign()

    def is_blackbox(self) -> bool:
        """
        :return: True if this is a blackbox.
        :rtype: bool
        """
        return self.__get_snl_model().isBlackBox()

    def is_leaf(self) -> bool:
        """
        :return: True if this is a leaf.
        :rtype: bool
        """
        return self.__get_snl_model().isLeaf()

    def is_const0(self) -> bool:
        """
        :return: True if this is a constant 0 generator.
        :rtype: bool
        """
        return self.__get_snl_model().isConst0()

    def is_const1(self) -> bool:
        """
        :return: True if this is a constant 1 generator.
        :rtype: bool
        """
        return self.__get_snl_model().isConst1()

    def is_const(self) -> bool:
        """
        :return: True if this is a constant generator.
        :rtype: bool
        """
        return self.__get_snl_model().isConst()

    def is_buf(self) -> bool:
        """
        :return: True if this is a buffer.
        :rtype: bool
        """
        return self.__get_snl_model().isBuf()

    def is_inv(self) -> bool:
        """
        :return: True if this is an inverter.
        :rtype: bool
        """
        return self.__get_snl_model().isInv()

    def __get_snl_model(self):
        if self.is_top():
            return naja.NLUniverse.get().getTopDesign()
        if (
            self.inst.getModel().getRevisionCount() != self.revisionCount or
            self.inst.getModel().getNLID() != self.SNLID
        ):
            self.inst = get_snl_instance_from_id_list(self.pathIDs)
            self.revisionCount = self.inst.getModel().getRevisionCount()
            self.SNLID = self.inst.getModel().getNLID()

        return self.inst.getModel()

    def __get_leaf_snl_object(self):
        if self.is_top():
            return naja.NLUniverse.get().getTopDesign()
        return get_snl_instance_from_id_list(self.pathIDs)

    def __find_snl_model(self, name: str) -> naja.SNLDesign:
        u = naja.NLUniverse.get()
        for db in u.getUserDBs():
            for lib in db.getLibraries():
                found_model = lib.getSNLDesign(name)
                if found_model is not None:
                    return found_model
        return None

    def dump_full_dot(self, path: str):
        """Dump the full dot file of this instance."""
        self.__get_snl_model().dumpFullDotFile(path)

    def dump_context_dot(self, path: str):
        self.__get_snl_model().dumpContextDotFile(path)

    def get_child_instance(self, names: Union[str, list]):
        """
        :param names: the name of the child instance
            or the path to the child Instance as a list of names.
        :return: the child Instance at the given path or None if it does not exist.
        :rtype: Instance or None
        """
        if isinstance(names, str):
            names = [names]
        if not names:
            raise ValueError("Names argument cannot be empty")
        model = self.__get_snl_model()
        path = self.pathIDs.copy()
        for name in names:
            childInst = model.getInstance(name)
            if childInst is None:
                return None
            path.append(childInst.getID())
            model = childInst.getModel()
        return Instance(path)

    def get_child_instance_by_id(self, ids: Union[int, list[int]]):
        """
        :param ids: the ID of the child instance
            or the path to the child Instance as a list of IDs.
        :return: the child Instance at the given path or None if it does not exist.
        :rtype: Instance or None
        """
        if isinstance(ids, int):
            ids = [ids]
        if not ids:
            raise ValueError("IDs argument cannot be empty")
        model = self.__get_snl_model()
        path = self.pathIDs.copy()
        for id in ids:
            childInst = model.getInstanceByID(id)
            if childInst is None:
                return None
            path.append(childInst.getID())
            model = childInst.getModel()
        return Instance(path)

    def get_child_instances(self):
        """Iterate over the child instances of this instance.
        Equivalent to go down one level in hierarchy.

        :return: an iterator over the child instances of this instance.
        :rtype: Iterator[Instance]
        """
        path = get_snl_path_from_id_list(self.pathIDs)
        for inst in self.__get_snl_model().getInstances():
            path_child = naja.SNLPath(path, inst)
            yield Instance(path_child)
            # path.pop()

    def count_child_instances(self) -> int:
        """
        :return: the number of child instances of this instance.
        :rtype: int
        """
        return sum(1 for _ in self.__get_snl_model().getInstances())

    def get_nets(self):
        """Iterate over all scalar nets and bus nets.

        :return: an iterator over the nets of this Instance.
        :rtype: Iterator[Net]
        """
        for net in self.__get_snl_model().getNets():
            yield Net(self.pathIDs, net)

    def count_nets(self) -> int:
        """Count the number of scalar nets and bus nets of this Instance.

        :return: the number of nets of this Instance.
        :rtype: int
        """
        return sum(1 for _ in self.get_nets())

    def get_bit_nets(self):
        """Iterate over all scalar nets and bus net bits.

        :return: an iterator over the nets at bit level of this Instance.
        :rtype: Iterator[Net]
        """
        for net in self.__get_snl_model().getNets():
            if isinstance(net, naja.SNLBusNet):
                for bit in net.getBits():
                    yield Net(self.pathIDs, bit)
            else:
                yield Net(self.pathIDs, net)

    def count_bit_nets(self) -> int:
        """Count the number of scalar nets and bus net bits of this Instance.

        :return: the number of bit nets of this Instance.
        :rtype: int
        """
        return sum(1 for _ in self.get_bit_nets())

    def get_net(self, name: str) -> Net:
        """
        :param str name: the name of the Net to get.
        :return: the Net with the given name or None if it does not exist.
        :rtype: Net or None
        """
        net = self.__get_snl_model().getNet(name)
        if net is not None:
            return Net(self.pathIDs, net)
        return None

    def is_primitive(self) -> bool:
        """
        :return: True if this is a primitive.
        :rtype: bool
        """
        return self.__get_snl_model().isPrimitive()

    def is_sequential(self) -> bool:
        """
        :return: True if this is a sequential element.
        :rtype: bool
        """
        return self.__get_snl_model().isSequential()

    def has_modeling(self) -> bool:
        """
        :return: True if this instance has timing modeling.
        :rtype: bool
        """
        return self.__get_snl_model().hasModeling()

    def get_terms(self):
        """Iterate over all scalar terms and bus terms of this Instance.

        :return: the terms of this Instance.
        :rtype: Iterator[Term]
        """
        for term in self.__get_snl_model().getTerms():
            yield Term(self.pathIDs, term)

    def count_terms(self) -> int:
        """Count the number of scalar terms and bus terms of this Instance.

        :return: the number of terms of this Instance.
        :rtype: int
        """
        return sum(1 for _ in self.get_terms())

    def get_bit_terms(self):
        """Iterate over all scalar terms and bus term bits.

        :return: the bit terms of this Instance.
        :rtype: Iterator[Term]
        """
        for term in self.__get_snl_model().getBitTerms():
            yield Term(self.pathIDs, term)

    def count_bit_terms(self) -> int:
        """Count the number of scalar terms and bus term bits of this Instance.

        :return: the number of bit terms of this Instance.
        :rtype: int
        """
        return sum(1 for _ in self.get_bit_terms())

    def get_term(self, name: str) -> Term:
        """
        :param str name: the name of the Term to get.
        :return: the Term with the given name.
        :rtype: Term or None
        """
        term = self.__get_snl_model().getTerm(name)
        if term is not None:
            return Term(self.pathIDs, self.__get_snl_model().getTerm(name))
        return None

    def get_input_terms(self):
        """Iterate over all scalar input terms and bus input terms
        of this Instance.

        :return: the input terms of this Instance.
        :rtype: Iterator[Term]
        """
        for term in self.__get_snl_model().getTerms():
            if term.getDirection() != naja.SNLTerm.Direction.Output:
                yield Term(self.pathIDs, term)

    def count_input_terms(self) -> int:
        """Count the number of scalar input terms and bus input terms
        of this Instance.

        :return: the number of input terms of this Instance.
        :rtype: int
        """
        return sum(1 for _ in self.get_input_terms())

    def get_input_bit_terms(self):
        """Iterate over all scalar input terms and bus input term bits
        of this Instance.

        :return: the bit input terms of this Instance.
        :rtype: Iterator[Term]
        """
        for term in self.__get_snl_model().getTerms():
            if term.getDirection() != naja.SNLTerm.Direction.Output:
                if isinstance(term, naja.SNLBusTerm):
                    for bit in term.getBits():
                        yield Term(self.pathIDs, bit)
                else:
                    yield Term(self.pathIDs, term)

    def count_input_bit_terms(self) -> int:
        """Count the number of scalar input terms and bus input term bits
        of this Instance.

        :return: the number of bit input terms of this Instance.
        :rtype: int
        """
        return sum(1 for _ in self.get_input_bit_terms())

    def get_output_terms(self):
        """Iterate over all scalar output terms and bus output terms
        of this Instance.

        :return: the output terms of this Instance.
        :rtype: Iterator[Term]
        """
        for term in self.__get_snl_model().getTerms():
            if term.getDirection() != naja.SNLTerm.Direction.Input:
                yield Term(self.pathIDs, term)

    def count_output_terms(self) -> int:
        """Count the number of scalar output terms and bus output terms
        of this Instance.

        :return: the number of output terms of this Instance.
        :rtype: int
        """
        return sum(1 for _ in self.get_output_terms())

    def get_output_bit_terms(self):
        """Iterate over all scalar output terms and bus output term bits
        of this Instance.

        :return: the bit output terms of this Instance.
        :rtype: Iterator[Term]
        """
        for term in self.__get_snl_model().getTerms():
            if term.getDirection() != naja.SNLTerm.Direction.Input:
                if isinstance(term, naja.SNLBusTerm):
                    for bit in term.getBits():
                        yield Term(self.pathIDs, bit)
                else:
                    yield Term(self.pathIDs, term)

    def count_output_bit_terms(self) -> int:
        """Count the number of scalar output terms and bus output term bits
        of this Instance.

        :return: the number of bit output terms of this Instance.
        :rtype: int
        """
        return sum(1 for _ in self.get_output_bit_terms())

    def get_attributes(self) -> Iterator[Attribute]:
        """Iterate over the attributes of this Instance.

        :return: the attributes of this Instance.
        :rtype: Iterator[Attribute]
        """
        leaf_object = self.__get_leaf_snl_object()
        for attribute in leaf_object.getAttributes():
            yield Attribute(attribute)

    def count_attributes(self) -> int:
        """Count the attributes of this Instance.

        :return: the number of attributes of this Instance.
        :rtype: int
        """
        return sum(1 for _ in self.get_attributes())

    def get_design(self) -> "Instance":
        """
        :return: the Instance containing this instance.
        :rtype: Instance
        """
        path = self.pathIDs.copy()
        if len(self.pathIDs) == 1:
            return get_top()
        path.pop()
        return Instance(path)

    def delete(self):
        """Delete this instance."""
        if self.is_top():
            raise ValueError("Cannot delete the top instance")
        # FIXME: should be upper path ?
        path = get_snl_path_from_id_list(self.pathIDs)
        inst = get_snl_instance_from_id_list(self.pathIDs)
        naja.SNLUniquifier(path)
        inst.destroy()

    def get_name(self) -> str:
        """
        :return: the name of the instance or name of the top is this is the top.
        :rtype: str
        """
        if self.is_top():
            return self.get_model_name()
        else:
            path = get_snl_path_from_id_list(self.pathIDs)
            return path.getTailInstance().getName()

    def set_name(self, name: str):
        """Set the name of this instance.

        :param str name: the new name of the instance.
        """
        if self.is_top():
            topSNLDesign = naja.NLUniverse.get().getTopDesign()
            topSNLDesign.setName(name)
        else:
            path = get_snl_path_from_id_list(self.pathIDs)
            # We need to uniquify until parent instance if parent instance
            # is not top. path.size == 1 for instance under top
            if path.size() > 1:
                naja.SNLUniquifier(path.getHeadPath())
                path = get_snl_path_from_id_list(self.pathIDs)
            inst = path.getTailInstance()
            inst.setName(name)

    def get_model_name(self) -> str:
        """
        :return: the name of the model of the instance
            or name of the top is this is the top.
        :rtype: str
        """
        return self.__get_snl_model().getName()

    def get_model_id(self) -> tuple[int, int, int]:
        """
        :return: the ID of the model of this Instance
            or ID of the top if this is the top.
        """
        model = self.__get_snl_model()
        return model.getDB().getID(), model.getLibrary().getID(), model.getID()

    def create_child_instance(self, model: str, name: str) -> "Instance":
        """Create a child instance with the given model and name.

        :param str model: the name of the model of the instance to create.
        :param str name: the name of the instance to create.
        :return: the created Instance.
        :rtype: Instance
        """
        path = get_snl_path_from_id_list(self.pathIDs)
        if path.size() > 0:
            naja.SNLUniquifier(path)
            path = get_snl_path_from_id_list(self.pathIDs)
        design = self.__get_snl_model()
        new_instance_model = self.__find_snl_model(model)
        if new_instance_model is None:
            raise ValueError(
                f"Cannot create instance {name} in {self}: model {model} cannot be found"
            )
        newSNLInstance = naja.SNLInstance.create(design, new_instance_model, name)
        path = naja.SNLPath(path, newSNLInstance)
        return Instance(path)

    def create_term(self, name: str, direction: Term.Direction) -> Term:
        """Create a Term in this Instance with the given name and direction.

        :param str name: the name of the Term to create.
        :param Term.Direction direction: the direction of the Term to create.
        :return: the created Term.
        """
        path = get_snl_path_from_id_list(self.pathIDs)
        if path.size() > 0:
            naja.SNLUniquifier(path)
            path = get_snl_path_from_id_list(self.pathIDs)
        design = self.__get_snl_model()
        newSNLTerm = naja.SNLScalarTerm.create(design, direction.value, name)
        return Term(path.getInstanceIDs(), newSNLTerm)

    def create_output_term(self, name: str) -> Term:
        """Create an output Term in this Instance with the given name.

        :param str name: the name of the Term to create.
        :return: the created Term.
        :rtype: Term
        """
        return self.create_term(name, Term.Direction.OUTPUT)

    def create_input_term(self, name: str) -> Term:
        """Create an input Term in this Instance with the given name.

        :param str name: the name of the Term to create.
        :return: the created Term.
        :rtype: Term
        """
        return self.create_term(name, Term.Direction.INPUT)

    def create_inout_term(self, name: str) -> Term:
        """Create an inout Term in this Instance with the given name.

        :param str name: the name of the Term to create.
        :return: the created Term.
        :rtype: Term
        """
        return self.create_term(name, Term.Direction.INOUT)

    def create_bus_term(self, name: str, msb: int, lsb: int, direction: Term.Direction) -> Term:
        """Create a bus Term in this Instance with the given name, msb, lsb and direction.

        :param str name: the name of the Term to create.
        :param int msb: the most significant bit of the Term to create.
        :param int lsb: the least significant bit of the Term to create.
        :param Term.Direction direction: the direction of the Term to create.
        :return: the created Term.
        """
        path = get_snl_path_from_id_list(self.pathIDs)
        if path.size() > 0:
            naja.SNLUniquifier(path)
        design = self.__get_snl_model()
        newSNLTerm = naja.SNLBusTerm.create(design, direction.value, msb, lsb, name)
        return Term(self.pathIDs, newSNLTerm)

    def create_inout_bus_term(self, name: str, msb: int, lsb: int) -> Term:
        """Create an inout bus Term in this Instance with the given name, msb and lsb.

        :param str name: the name of the Term to create.
        :param int msb: the most significant bit of the Term to create.
        :param int lsb: the least significant bit of the Term to create.
        :return: the created Term.
        :rtype: Term
        """
        return self.create_bus_term(name, msb, lsb, Term.Direction.INOUT)

    def create_output_bus_term(self, name: str, msb: int, lsb: int) -> Term:
        """Create an output bus Term in this Instance with the given name, msb and lsb.

        :param str name: the name of the Term to create.
        :param int msb: the most significant bit of the Term to create.
        :param int lsb: the least significant bit of the Term to create.
        :return: the created Term.
        :rtype: Term
        """
        return self.create_bus_term(name, msb, lsb, Term.Direction.OUTPUT)

    def create_input_bus_term(self, name: str, msb: int, lsb: int) -> Term:
        """Create an input bus Term in this Instance with the given name, msb and lsb.

        :param str name: the name of the Term to create.
        :param int msb: the most significant bit of the Term to create.
        :param int lsb: the least significant bit of the Term to create.
        :return: the created Term.
        :rtype: Term
        """
        return self.create_bus_term(name, msb, lsb, Term.Direction.INPUT)

    def create_net(self, name: str) -> Net:
        """Create a scalar Net in this Instance with the given name.

        :param str name: the name of the Net to create.
        :return: the created Net.
        :rtype: Net
        """
        path = get_snl_path_from_id_list(self.pathIDs)
        if path.size():
            naja.SNLUniquifier(path)
            path = get_snl_path_from_id_list(self.pathIDs)
        model = self.__get_snl_model()
        newSNLNet = naja.SNLScalarNet.create(model, name)
        return Net(path, newSNLNet)

    def create_bus_net(self, name: str, msb: int, lsb: int) -> Net:
        """Create a bus Net in this Instance with the given name, msb and lsb.

        :param str name: the name of the Net to create.
        :param int msb: the most significant bit of the Net to create.
        :param int lsb: the least significant bit of the Net to create.
        :return: the created Net.
        :rtype: Net
        """
        path = get_snl_path_from_id_list(self.pathIDs)
        if path.size():
            naja.SNLUniquifier(path)
            path = get_snl_path_from_id_list(self.pathIDs)
        model = self.__get_snl_model()
        newSNLNet = naja.SNLBusNet.create(model, msb, lsb, name)
        return Net(path, newSNLNet)

    def dump_verilog(self, path: str):
        """Dump the verilog of this instance.

        :param str path: the file path where to dump the verilog.
        :rtype: None
        :raises ValueError: if the path does not end with .v.
        :raises FileNotFoundError: if the directory of the path does not exist.
        """
        # path should be a file path of the form "path/to/file.v"
        if not path.endswith(".v"):
            raise ValueError("The path must end with .v")
        dir_path = os.path.dirname(path) or "."
        if not os.path.exists(dir_path):
            raise FileNotFoundError(f"The directory {dir_path} does not exist")
        self.__get_snl_model().dumpVerilog(dir_path, os.path.basename(path))

    def get_truth_table(self):
        """
        :return: the truth table of the instance.
        :rtype: list[str]
        """
        return self.__get_snl_model().getTruthTable()

    def add_clock_related_inputs(self, clock_term: Term, input_terms: List[Term]):
        """Add input terms that are related to the given clock term.

        :param clock_term: the clock term to check for related inputs.
        :param input_terms: a list of input terms to add.
        :return: None
        """
        snlterms = [get_snl_term_for_ids(term.pathIDs, term.termIDs) for term in input_terms]
        self.__get_snl_model().addInputsToClockArcs(snlterms,
                                                    get_snl_term_for_ids(clock_term.pathIDs,
                                                                         clock_term.termIDs))

    def add_clock_related_outputs(self, clock_term: Term, output_terms: List[Term]):
        """Add output terms that are related to the given clock term.

        :param clock_term: the clock term to check for related outputs.
        :param output_terms: a list of output terms to add.
        :return: None
        """
        # convert Term objects to SNL terms
        snlterms = [get_snl_term_for_ids(term.pathIDs, term.termIDs) for term in output_terms]
        self.__get_snl_model().addClockToOutputsArcs(
            get_snl_term_for_ids(clock_term.pathIDs, clock_term.termIDs), snlterms)

    def add_combinatorial_arcs(self, input_terms: List[Term], output_terms: List[Term]):
        """Add input terms that are combinatorial inputs for the given output term.

        :param output_term: the output term to check for combinatorial inputs.
        :param input_terms: a list of input terms to add.
        :return: None
        """
        self.__get_snl_model().addCombinatorialArcs(
            [get_snl_term_for_ids(term.pathIDs, term.termIDs) for term in input_terms],
            [get_snl_term_for_ids(term.pathIDs, term.termIDs) for term in output_terms])


def __get_top_db() -> naja.NLDB:
    if naja.NLUniverse.get() is None:
        naja.NLUniverse.create()
    if naja.NLUniverse.get().getTopDB() is None:
        db = naja.NLDB.create(naja.NLUniverse.get())
        naja.NLUniverse.get().setTopDB(db)
    return naja.NLUniverse.get().getTopDB()


def reset():
    """Reset the environment by deleting everything.
    :rtype: None
    """
    u = naja.NLUniverse.get()
    if u is not None:
        u.destroy()


def get_top():
    """
    :return: the top Instance.
    :rtype: Instance
    """
    return Instance(naja.SNLPath())


def create_top(name: str) -> Instance:
    """Create a top instance with the given name.

    :param str name: the name of the top instance to create.
    :return: the created top Instance.
    :rtype: Instance
    """
    # init
    db = __get_top_db()
    # create top design
    lib = naja.NLLibrary.create(db)
    top = naja.SNLDesign.create(lib, name)
    naja.NLUniverse.get().setTopDesign(top)
    return Instance()


@dataclass
class VerilogConfig:
    keep_assigns: bool = True
    allow_unknown_designs: bool = False


def load_verilog(files: Union[str, List[str]], config: VerilogConfig = None) -> Instance:
    """Load verilog files into the top design.

    :param files: a list of verilog files to load or a single file.
    :param config: the configuration to use when loading the files.
    :return: the top Instance.
    :rtype: Instance
    :raises Exception: if no files are provided.
    """
    if isinstance(files, str):
        files = [files]
    if not files or len(files) == 0:
        raise Exception("No verilog files provided")
    if config is None:
        config = VerilogConfig()  # Use default settings
    start_time = time.time()
    logging.info(f"Loading verilog: {', '.join(files)}")
    __get_top_db().loadVerilog(
        files,
        keep_assigns=config.keep_assigns,
        allow_unknown_designs=config.allow_unknown_designs
    )
    execution_time = time.time() - start_time
    logging.info(f"Loading done in {execution_time:.2f} seconds")
    return get_top()


def load_liberty(files: Union[str, List[str]]):
    """Load liberty files.

    :param files: a list of liberty files to load or a single file.
    :rtype: None
    :raises Exception: if no liberty files are provided.
    """
    if isinstance(files, str):
        files = [files]
    if not files or len(files) == 0:
        raise Exception("No liberty files provided")
    logging.info(f"Loading liberty files: {', '.join(files)}")
    __get_top_db().loadLibertyPrimitives(files)


def load_primitives(name: str):
    """Loads a primitive library embedded in najaeda.

    Currently supported libraries are:

    - xilinx
    - yosys
    :param str name: the name of the primitives library to load.
    :raises ValueError: if the name is not recognized.
    :rtype: None
    """
    if name == "xilinx":
        from najaeda.primitives import xilinx
        xilinx.load(__get_top_db())
    elif name == "yosys":
        from najaeda.primitives import yosys
        yosys.load(__get_top_db())
    else:
        raise ValueError(f"Unknown primitives library: {name}")


def load_primitives_from_file(file: str):
    """Loads a primitives library from a file.

    :param str file: the path to the primitives library file.
    The file must define a function `load(db)`.
    """
    logging.info(f"Loading primitives from file: {file}")
    if not os.path.isfile(file):
        raise FileNotFoundError(f"Cannot load primitives from non existing file: {file}")
    import importlib.util
    spec = importlib.util.spec_from_file_location("user_module", file)
    module = importlib.util.module_from_spec(spec)
    sys.modules["user_module"] = module
    spec.loader.exec_module(module)

    if not hasattr(module, "load"):
        raise RuntimeError(f"The file {file} must define a function named `load(db)`")

    db = __get_top_db()
    module.load(db)


def load_naja_if(path: str):
    """Load the Naja IF from the given path."""
    if not os.path.isdir(path):
        raise FileNotFoundError(f"Cannot load Naja IF from non existing directory: {path}")
    logging.info(f"Loading Naja IF from {path}")
    naja.NLDB.loadNajaIF(path)


def dump_naja_if(path: str):
    """Dump the Naja IF to the given path."""
    __get_top_db().dumpNajaIF(path)


def get_primitives_library() -> naja.NLLibrary:
    lib = __get_top_db().getLibrary("PRIMS")
    if lib is None:
        lib = naja.NLLibrary.createPrimitives(__get_top_db(), "PRIMS")
    return lib


def get_model_name(id: tuple[int, int, int]) -> str:
    """
    :param tuple[int, int, int] id: the id of the model.
    :return: the name of the model given its id or None if it does not exist.
    :rtype: str or None
    """
    u = naja.NLUniverse.get()
    if u:
        db = u.getDB(id[0])
        if db:
            lib = db.getLibrary(id[1])
            if lib:
                model = lib.getSNLDesign(id[2])
                if model:
                    return model.getName()
    return None


def apply_dle():
    """Apply the DLE (Dead Logic Elimination) to the top design.
    :rtype: None
    """
    top = naja.NLUniverse.get().getTopDesign()
    if top is not None:
        naja.NLUniverse.get().applyDLE()


def apply_constant_propagation():
    """Apply constant propagation to the top design.
    :rtype: None
    """
    top = naja.NLUniverse.get().getTopDesign()
    if top is not None:
        naja.NLUniverse.get().applyConstantPropagation()


def get_max_fanout() -> list:
    """Get the maximum fanout of the top design.

    :return: the maximum fanout of the top design.
    :rtype: int
    """
    u = naja.NLUniverse.get()
    if u is not None:
        top = naja.NLUniverse.get().getTopDesign()
        if top is not None:
            max_fanout = naja.NLUniverse.get().getMaxFanout()
            result = []
            index = 0
            result.append(max_fanout[0])
            fanouts = []
            for entry in max_fanout[1]:
                fanout = []
                driver = entry[0]
                component = driver.getNetComponent()
                path = driver.getPath().getInstanceIDs()
                if isinstance(component, naja.SNLInstTerm):
                    path.append(component.getInstance().getID())
                    component = component.getBitTerm()
                print(component)
                print(path)
                fanout.append(Term(path, component))
                readers = []
                for item in entry[1]:
                    component = item.getNetComponent()
                    path = item.getPath().getInstanceIDs()
                    if isinstance(component, naja.SNLInstTerm):
                        path.append(component.getInstance().getID())
                        component = component.getBitTerm()
                    readers.append(Term(path, component))
                fanout.append(readers)
                fanouts.append(fanout)
                index += 1
            result.append(fanouts)
            return result
    return [0]


def get_max_logic_level() -> list:
    """Get the maximum logic level of the top design.

    :return: the maximum logic level of the top design.
    :rtype: int
    """
    u = naja.NLUniverse.get()
    if u is not None:
        top = naja.NLUniverse.get().getTopDesign()
        if top is not None:
            max_logic_level = naja.NLUniverse.get().getMaxLogicLevel()
            result = []
            index = 0
            for entry in max_logic_level:
                if index == 0:
                    result.append(max_logic_level[0])
                else:
                    paths = []
                    for path in entry:
                        llpath = []
                        for item in path:
                            component = item.getNetComponent()
                            pathIDs = item.getPath().getInstanceIDs()
                            if isinstance(component, naja.SNLInstTerm):
                                pathIDs.append(component.getInstance().getID())
                                component = component.getBitTerm()
                            llpath.append(Term(pathIDs, component))
                        paths.append(llpath)
                    result.append(paths)
                index += 1
            return result
    return [0]
