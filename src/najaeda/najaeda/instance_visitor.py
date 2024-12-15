# SPDX-FileCopyrightText: 2024 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

from typing import Callable
from najaeda import netlist


class VisitorConfig:
    def __init__(
        self,
        enter_condition: Callable[[netlist.Instance], bool] = lambda node: True,
        callback: Callable[[netlist.Instance], None] = lambda node: None,
    ):
        """
        :param enter_condition: A function that determines whether to visit
        the children of an instance.
        :param callback: The callback to be executed when an instance is visited.
        """
        self.callback = callback
        self.enter_condition = enter_condition


class Visitor:
    def __init__(self, instance: netlist.Instance):
        """
        :param netlist: The hierarchical netlist to be traversed.
        """
        self.instance = instance

    def visit(self, instance: netlist.Instance, config: VisitorConfig):
        """
        Recursively visits nodes in the netlist hierarchy.

        :param instance: The current node in the netlist instance hierarchy.
        :param config: VisitorConfig object defining conditions and callbacks.
        """
        # Execute the callback
        config.callback(instance)

        # Check if we should proceed to children
        if config.enter_condition(instance):
            for child in instance.get_child_instances():
                self.visit(child, config)
