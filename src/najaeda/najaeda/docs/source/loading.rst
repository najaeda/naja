Loading and Saving
==================

The top-level load and dump functions live in :mod:`najaeda.netlist`.

Session lifecycle
-----------------

.. code-block:: python

   from najaeda import netlist

   netlist.reset()
   top = netlist.create_top("top")

Call :func:`najaeda.netlist.reset` at the beginning of scripts and tests when
you need an empty universe.  Loading a design returns the top
:class:`najaeda.netlist.Instance` wrapper.

Verilog
-------

.. code-block:: python

   from najaeda import netlist

   config = netlist.VerilogConfig()
   top = netlist.load_verilog(["cells.v", "design.v"], config)

Use Verilog loading for structural Verilog netlists.  Load Liberty or
primitive libraries first when the design instantiates standard cells or
technology primitives.

SystemVerilog
-------------

.. code-block:: python

   from najaeda import netlist

   config = netlist.SystemVerilogConfig()
   top = netlist.load_system_verilog(["pkg.sv", "rtl.sv"], config)

SystemVerilog loading elaborates the design through the native frontend and
then builds SNL objects.  Frontend diagnostics are raised as native
``SystemVerilog*`` exceptions from :mod:`najaeda.naja`.

By default, an incremental diagnostics report is written to
``naja_sv_diagnostics.log``. Set ``diagnostics_report_path=None`` in
:class:`najaeda.netlist.SystemVerilogConfig` to disable that file and retain
console diagnostics only.

Liberty and primitive libraries
-------------------------------

.. code-block:: python

   from najaeda import netlist

   netlist.load_liberty(["NangateOpenCellLibrary_typical.lib"])
   netlist.load_primitives("xilinx")

The primitives library is available through
:func:`najaeda.netlist.get_primitives_library` for scripts that need to inspect
or create primitive-backed models.

Naja interchange
----------------

.. code-block:: python

   from najaeda import netlist

   top = netlist.load_naja_if("design.naja")
   netlist.dump_naja_if("roundtrip.naja")

Naja interchange preserves the SNL database more directly than Verilog and is
useful for round-tripping internal analyses and transformations.

Verilog output
--------------

Use native dump methods when you need Verilog output from the underlying SNL
database.  The high-level package currently exposes Naja interchange directly;
for raw Verilog dump control, see :doc:`raw_api`.
