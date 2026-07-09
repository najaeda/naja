Quick Start
===========

This page shows the shape of a typical :mod:`najaeda.netlist` script.

Load a design
-------------

.. code-block:: python

   from najaeda import netlist

   netlist.reset()
   netlist.load_liberty(["NangateOpenCellLibrary_typical.lib"])
   top = netlist.load_verilog("counter.v")

   print(top.get_name())

Browse hierarchy
----------------

.. code-block:: python

   for child in top.get_child_instances():
       print(child.get_name(), child.get_model_name())

   for leaf in top.get_leaf_instances():
       print("leaf", leaf.get_name(), leaf.get_model_name())

Inspect ports and nets
----------------------

.. code-block:: python

   for term in top.get_terms():
       print(term.get_name(), term.get_direction(), term.get_width())

   clk = top.get_term("clk")
   print(clk.get_net())

Follow flat connectivity
------------------------

Use an equipotential when you want connectivity across hierarchy boundaries.

.. code-block:: python

   reset_equi = top.get_term("reset").get_equipotential()

   print("drivers")
   for term in reset_equi.get_leaf_drivers():
       print(term)

   print("readers")
   for term in reset_equi.get_leaf_readers():
       print(term)

Edit and save
-------------

.. code-block:: python

   old = top.get_net("old_name")
   old.set_name("renamed_signal")

   netlist.dump_naja_if("edited.naja")

The high-level API automatically uniquifies shared models before edits that
must be localized to a hierarchical occurrence.
