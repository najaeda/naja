najaeda
=======

.. image:: https://img.shields.io/pypi/v/najaeda
   :target: https://pypi.org/project/najaeda/
   :alt: PyPI version

.. image:: https://img.shields.io/badge/License-Apache_2.0-blue.svg
   :target: https://opensource.org/licenses/Apache-2.0
   :alt: Apache 2.0 License

.. image:: https://img.shields.io/badge/Matrix-Join%20Chat-success?logo=matrix
   :target: https://matrix.to/#/#naja:fossi-chat.org
   :alt: Join the Matrix chat

----

**najaeda** is a Python package for loading, elaborating, navigating, editing,
and analysing hardware designs from simple Verilog netlists to large
SystemVerilog RTL cores.

.. code-block:: python

    from najaeda import netlist

    # Load a gate-level Verilog design with a Liberty standard-cell library
    netlist.load_liberty(['NangateOpenCellLibrary.lib'])
    top = netlist.load_verilog('my_design.v')

    # Navigate the hierarchy
    for inst in top.get_child_instances():
        print(f'{inst.get_name()} → {inst.get_model_name()}')

    # Flat connectivity across hierarchy boundaries
    for iterm in top.get_term('clk').get_equipotential().get_inst_terms():
        print(iterm)

    # Edit: rename, reconnect, delete
    top.get_net('old_name').set_name('new_name')


What you can do
---------------

- **Load** Verilog and elaborated SystemVerilog designs, with or without
  Liberty standard-cell libraries
- **Navigate** hierarchy, nets, and ports at any level of detail —
  instance-by-instance or flat via equipotentials
- **Edit** netlists: rename instances and nets, disconnect and reconnect
  signals, delete logic
- **Analyse** designs with the visitor API and export results to pandas
  for further processing or visualisation

Installation
------------

.. code-block:: bash

    pip install najaeda

Requires Python 3.8+. Wheels are published for Linux, macOS, and Windows.

Tutorials
---------

Six hands-on notebooks — open any of them in Google Colab with no local
install needed:

.. list-table::
   :header-rows: 1
   :widths: 5 60 15

   * - #
     - Topic
     - Colab
   * - 1
     - Getting started — load Verilog, navigate hierarchy, visualize
     - .. image:: https://colab.research.google.com/assets/colab-badge.svg
          :target: https://colab.research.google.com/github/najaeda/naja/blob/main/tutorials/notebooks/01_getting_started.ipynb
          :alt: Open in Colab
   * - 2
     - Liberty primitives — load a synthesised design with standard cells
     - .. image:: https://colab.research.google.com/assets/colab-badge.svg
          :target: https://colab.research.google.com/github/najaeda/naja/blob/main/tutorials/notebooks/02_liberty_primitives_design.ipynb
          :alt: Open in Colab
   * - 3
     - Editing a netlist — rename, disconnect, reconnect, delete
     - .. image:: https://colab.research.google.com/assets/colab-badge.svg
          :target: https://colab.research.google.com/github/najaeda/naja/blob/main/tutorials/notebooks/03_editing_a_netlist.ipynb
          :alt: Open in Colab
   * - 4
     - SystemVerilog elaboration — load and browse an elaborated SV design
     - .. image:: https://colab.research.google.com/assets/colab-badge.svg
          :target: https://colab.research.google.com/github/najaeda/naja/blob/main/tutorials/notebooks/04_systemverilog_elaborated_netlist.ipynb
          :alt: Open in Colab
   * - 5
     - ibex RISC-V core — explore a real-world SV core, collect module stats
     - .. image:: https://colab.research.google.com/assets/colab-badge.svg
          :target: https://colab.research.google.com/github/najaeda/naja/blob/main/tutorials/notebooks/05_ibex_riscv_core.ipynb
          :alt: Open in Colab
   * - 6
     - Fanout analysis — compute fanout for every net, trace drivers, export
       to pandas
     - .. image:: https://colab.research.google.com/assets/colab-badge.svg
          :target: https://colab.research.google.com/github/najaeda/naja/blob/main/tutorials/notebooks/06_fanout_analysis.ipynb
          :alt: Open in Colab

Links
-----

- `API documentation <https://najaeda.readthedocs.io/en/latest/>`_
- `GitHub repository <https://github.com/najaeda/naja>`_
- `Issue tracker <https://github.com/najaeda/naja/issues>`_
- `Matrix chat <https://matrix.to/#/#naja:fossi-chat.org>`_
- `contact@keplertech.io <mailto:contact@keplertech.io>`_

License
-------

Apache License 2.0.
See the `LICENSE <https://github.com/najaeda/naja/blob/main/LICENSE>`_ file for details.
