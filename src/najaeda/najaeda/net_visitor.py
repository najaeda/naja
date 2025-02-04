# SPDX-FileCopyrightText: 2024 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

# from typing import Callable
# from najaeda import netlist
#
#
# class VisitorConfig:
#     def __init__(
#         self,
#         enter_condition: Callable[[netlist.Instance], bool] = lambda node: True,
#         callback: Callable[..., None] = lambda node, *args, **kwargs: None,
#         args: tuple = (),
#         kwargs: dict = None,
#     ):
#         """
#         :param enter_condition: A callable that takes a node (dict)
#             and returns True if the visitor should enter.
#         :param callback: A callable that takes a node (dict) and performs an operation on it.
#         :param args: Positional arguments to pass to the callback.
#         :param kwargs: Keyword arguments to pass to the callback.
#         """
#         self.enter_condition = enter_condition
#         self.callback = callback
#         self.args = args
#         self.kwargs = kwargs or {}
#
#
# def visit(equipotential: netlist.Equipotential, config: VisitorConfig):
#     """Recursively visits nodes in the netlist hierarchy.
#
#     :param instance: The current node in the netlist instance hierarchy.
#     :param config: VisitorConfig object defining conditions and callbacks.
#     """
#     # Execute the callback
#     config.callback(equipotential, *config.args, **config.kwargs)
#
#     # Check if we should proceed to children
#     if config.enter_condition(equipotential):
#         for component in equipotential.get .get_child_instances():
#             visit(child, config)
#
# def visit(term: netlist.Term, config: VisitorConfig):
#     config.callback(term, *config.args, **config.kwargs)
#
#     #get the corresponding iterms
#     instance = term.get_instance()
#     if instance is not None:
#         for iterm in instance.get_iterms():
#             visit(iterm, config)
#
