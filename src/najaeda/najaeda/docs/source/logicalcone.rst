Logical Cone Class
==================

Logical Cone Overview
---------------------

``naja.LogicCone`` builds the combinational fan-in or fan-out cone of a
net-component occurrence.  The result is a rooted DAG whose internal nodes are
crossed leaf-cell occurrences and whose frontier contains sequential cells,
top-level ports, and opaque black boxes.

Python API
----------

Construct a cone with ``LogicCone(start, direction)``.  ``start`` is one
``SNLOccurrence`` referencing a scalar term, one bit of a bus, or a bus term.
A bus-term start builds one shared cone for all of its bits.  Direction is
``LogicCone.FanIn`` or ``LogicCone.FanOut``.

``get_nodes()``, ``get_root()``, and ``get_leaves()`` return node tuples:

``(node_id, occurrence, kind, next_ids, prev_ids)``

Kinds are ``"root"``, ``"internal"``, ``"flop"``, ``"ports"``, and
``"blackbox"``.  Edges point away from the root toward the cone frontier.
