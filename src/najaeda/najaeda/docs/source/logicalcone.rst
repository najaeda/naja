Logical Cone Class
==================

Logical Cone Overview
---------------------

``naja.SNLLogicalCone`` builds the combinational fan-in or fan-out cone of one
single-bit net-component occurrence.  The result is a rooted DAG whose internal
nodes are crossed leaf-cell occurrences and whose frontier contains sequential
cells, top-level ports, and opaque black boxes.

Python API
----------

Construct a cone with ``SNLLogicalCone(start, direction)``.  ``start`` is one
``SNLOccurrence`` referencing a scalar term or one bit of a bus.  A caller
wanting a bus cone constructs one ``SNLLogicalCone`` per bit.  Direction is
``SNLLogicalCone.FanIn`` or ``SNLLogicalCone.FanOut``.

``get_nodes()``, ``get_root()``, and ``get_leaves()`` return node tuples:

``(node_id, occurrence, kind, next_ids, prev_ids)``

Kinds are ``"root"``, ``"internal"``, ``"flop"``, ``"ports"``, and
``"blackbox"``.  Edges point away from the root toward the cone frontier.
