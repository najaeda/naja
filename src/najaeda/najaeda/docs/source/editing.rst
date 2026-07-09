Editing Netlists
================

The high-level API is intended for ECO-style edits performed from Python.
Objects are wrappers around native SNL objects plus hierarchical path
information, so edits can be localized to one occurrence when the design model
is shared.

Names
-----

.. code-block:: python

   net = top.get_net("n42")
   net.set_name("carry_out")

   inst = top.get_child_instance("u0")
   inst.set_name("u_adder0")

Connections
-----------

Terms and instance terms can be disconnected and reconnected through their
net-level methods.  Prefer the high-level :class:`najaeda.netlist.Term` and
:class:`najaeda.netlist.Net` wrappers when editing hierarchical designs,
because they preserve occurrence context.

.. code-block:: python

   src = top.get_net("new_clk")
   term = top.get_child_instance("u_reg").get_term("CLK")
   term.set_net(src)

Deletion
--------

Delete objects through the wrapper that represents the selected hierarchical
context.  After deletion, do not keep using stale Python objects that pointed
to the destroyed native SNL object.

.. code-block:: python

   dead = top.get_child_instance("u_unused")
   dead.destroy()

Uniquification
--------------

When a model is instantiated multiple times, mutating the model directly would
affect every occurrence.  The high-level API handles this by uniquifying the
path before context-sensitive edits.

If you bypass :mod:`najaeda.netlist` and edit raw :mod:`najaeda.naja` objects
directly, you are responsible for preserving this behavior yourself.

Optimization passes
-------------------

The package exposes native optimization passes for common cleanups:

.. code-block:: python

   from najaeda import netlist

   netlist.apply_constant_propagation()
   netlist.apply_dle()

Run analyses again after optimization passes, because object reachability and
connectivity may have changed.
