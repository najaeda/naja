.. SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
.. SPDX-License-Identifier: Apache-2.0

Interactive schematics
======================

The optional ``najaeda.schematic`` module exposes the
`naja-schematic viewer <https://github.com/najaeda/naja-schematic>`_.
Use it to explore the hierarchy and connectivity of a loaded design in a
Jupyter, Google Colab, or VS Code notebook. The viewer runs in the cell output
and queries the live netlist in the Python kernel.

Installation
------------

Install the schematic extra in the environment used by your notebook kernel:

.. code-block:: console

   python -m pip install "najaeda[schematic]"

In a notebook, use ``%pip install "najaeda[schematic]"`` instead. Restart the
kernel if needed after installation. The extra installs ``naja-schematic``;
the base ``najaeda`` package does not require the viewer.

Display a loaded design
-----------------------

Load or create a design first, using the same APIs as in :doc:`loading` and
:doc:`editing`. For example, to inspect a gate-level Verilog design:

.. code-block:: python

   from najaeda import netlist
   from najaeda.schematic import show

   netlist.load_liberty(["cells.lib"])
   top = netlist.load_verilog("design.v")

   view = show(height=700)
   view

Replace the filenames with your design and its cell library. Designs loaded
with the SystemVerilog loader or created in Python can also be viewed.
``show()`` uses the current top design; it does not take a filename or a
``netlist.Instance`` as its first argument.

Keep ``view`` as the last expression in the cell, or call
``IPython.display.display(view)`` explicitly. Expand the hierarchy tree to
inspect instances and ports, and explore connected logic in the schematic
and equipotential views. The default viewer height is 600 pixels.

After editing or replacing the netlist, call ``show()`` again for a fresh view.
Keep the kernel running: the widget needs it to answer netlist queries.

Focus and selection
-------------------

With ``naja-schematic`` 0.1.2 or later, you can start on a particular instance
and access the selection from Python. Upgrade with
``%pip install --upgrade "naja-schematic>=0.1.2"`` if needed.

.. code-block:: python

   view = show(instance="u_sub/u_and")
   view

In a later cell, focus another instance or read the user's selection:

.. code-block:: python

   view.show_instance(["u_sub", "u_or"])
   selected = view.selected
   if selected is not None:
       print(selected.get_model_name())

Paths contain instance names, excluding the top design name. Use names from
your own hierarchy. ``show_instance()`` also accepts a ``netlist.Instance``;
an empty path selects the top design. The focused instance is drawn with its
pins so you can extend the schematic by following connections.
``view.on_select(callback)`` calls ``callback(instance)`` when the selection
changes; the instance may be ``None``.

Overlay diagnoses
-----------------

Attach analysis results to instances without changing the netlist:

.. code-block:: python

   items = [
       {
           "kind": "instance",
           "path": ["u_sub"],
           "severity": "warning",
           "message": "Review this instance",
           "source": "my-analysis",
       }
   ]
   view = show(diagnosis=items)
   view

The path is relative to the top design and must identify an instance in your
design. Severities are ``info``, ``warning``, and ``error``. To update an
existing view, call ``view.annotate(items)``; to clear its diagnoses, call
``view.annotate([])``. Both APIs also accept a dictionary containing an
``items`` list, such as a decoded diagnosis JSON document.

Optional API exports
--------------------

``najaeda.schematic`` re-exports these objects from ``naja_schematic``:

* ``show(height=600, diagnosis=None)`` returns a notebook widget. Version
  0.1.2 adds the optional ``instance`` argument described above.
* ``diagnosis_response(items)`` builds a diagnosis response dictionary for
  a list of items or an ``{"items": [...]}`` document. It does not display
  or send the response.
* ``handle_request(request)`` accepts a decoded viewer request dictionary
  and returns a list of response dictionaries, in send order. This is for
  custom viewer transports; normal notebook use calls ``show()``.
* ``__version__`` reports the installed viewer version, independently of
  the ``najaeda`` version.

The viewer and its protocol are developed in the separate
`naja-schematic project <https://github.com/najaeda/naja-schematic>`_.
Its interfaces may evolve; consult that project's documentation for custom
transport integration and the full diagnosis format.

Troubleshooting
---------------

* If importing ``najaeda.schematic`` asks you to install the extra, install
  it in the active kernel's environment and retry the import.
* If the output is blank, check that a top design is loaded, display the
  returned widget, and check that the notebook supports widgets. Inspect
  the kernel output and browser developer console for errors.
* If the viewer reports a missing bundle, install a released
  ``naja-schematic`` wheel. A source checkout requires building its viewer
  assets as described in the upstream development instructions.
