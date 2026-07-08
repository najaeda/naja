Core Concepts
=============

``najaeda`` uses Naja's SNL database model.  The high-level
:mod:`najaeda.netlist` API keeps the main SNL concepts visible while hiding
most raw database plumbing.

Database, libraries, and designs
--------------------------------

A session contains one native :class:`najaeda.naja.NLUniverse`.  The universe
owns databases, databases own libraries, and libraries own designs.

For normal scripts, :mod:`najaeda.netlist` manages this structure:

* :func:`najaeda.netlist.reset` clears the current universe.
* :func:`najaeda.netlist.create_top` creates a new top design.
* :func:`najaeda.netlist.get_top` returns the current top design wrapper.
* load functions such as :func:`najaeda.netlist.load_verilog` create or update
  the top database and return the top :class:`najaeda.netlist.Instance`.

Instances and models
--------------------

An :class:`najaeda.netlist.Instance` represents an SNL instance in its
hierarchical context.  The instance has a model design, a name, child
instances, terms, and nets.  A child instance can itself contain more child
instances when its model is hierarchical.

The same model can be instantiated several times.  Editing one occurrence may
therefore require uniquification so that the edit applies only to the selected
hierarchical context.  The high-level API performs this automatically for its
editing methods.

Terms, nets, and bits
---------------------

Verilog ports are represented by :class:`najaeda.netlist.Term` objects.
Internal signals are represented by :class:`najaeda.netlist.Net` objects.
Both can be scalar, full buses, or individual bus bits.

The high-level wrappers preserve this distinction:

* scalar objects represent a single bit;
* bus objects represent a complete vector;
* bus-bit objects represent one bit of a vector;
* some net queries can return a concatenation wrapper when a connection maps
  to several bits.

Connectivity
------------

Local connectivity answers questions inside one design model: which net is
connected to this term, which instance terms are attached to this net, and so
on.

An :class:`najaeda.netlist.Equipotential` answers the flat connectivity
question across hierarchy boundaries.  It can enumerate top terms, leaf
drivers, leaf readers, and instance terms connected to a signal.

Source locations and attributes
-------------------------------

Many objects expose source ranges and attributes when they were produced by a
frontend that preserved this information.  Use source ranges for diagnostics
and attributes for metadata imported from HDL or attached by analysis passes.

Choosing an API level
---------------------

Use :mod:`najaeda.netlist` for application code.  Use :mod:`najaeda.naja` only
when you need raw SNL construction, exact native object access, live
SystemVerilog frontend intent data, or a native method that is not wrapped yet.
See :doc:`raw_api` for the expert workflow.
