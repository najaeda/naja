najaeda documentation
=====================

``najaeda`` is the Python interface to Naja's structured netlist model.
It is designed for scripts and tools that load Verilog, elaborate
SystemVerilog RTL, browse hierarchy and connectivity, compute design metrics,
and make controlled ECO-style edits.

The package has two Python API levels:

* :mod:`najaeda.netlist` is the recommended public API.  It wraps the native
  SNL objects with Pythonic, hierarchy-aware classes such as
  :class:`najaeda.netlist.Instance`, :class:`najaeda.netlist.Net`, and
  :class:`najaeda.netlist.Term`.
* :mod:`najaeda.naja` is the raw compiled extension module.  It exposes the
  underlying SNL objects almost directly and is intended for experts,
  debugging, and features that have not yet been wrapped by
  :mod:`najaeda.netlist`.

Most users should start with the high-level API and drop to the raw API only
when they need exact SNL semantics or access to a native feature not yet
available through :mod:`najaeda.netlist`.

.. toctree::
   :maxdepth: 2
   :caption: User guide:

   introduction
   concepts
   quickstart
   loading
   editing
   netlist_classes
   common_classes
   visitors
   examples

.. toctree::
   :maxdepth: 2
   :caption: API reference:

   api
   raw_api
