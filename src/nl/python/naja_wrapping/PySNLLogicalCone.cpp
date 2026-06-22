// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLLogicalCone.h"

#include <sstream>
#include <string>

#include "PyInterface.h"
#include "PySNLOccurrence.h"
#include "SNLLogicalCone.h"

namespace PYNAJA {

using namespace naja::NL;

static const char* nodeKindString(SNLLogicalCone::NodeKind kind) {
  switch (kind) {
    case SNLLogicalCone::NodeKind::Root: return "root";
    case SNLLogicalCone::NodeKind::Internal: return "internal";
    case SNLLogicalCone::NodeKind::Flop: return "flop";
    case SNLLogicalCone::NodeKind::Ports: return "ports";
    case SNLLogicalCone::NodeKind::Blackbox: return "blackbox";
  }
  return "unknown"; // LCOV_EXCL_LINE defensive: all NodeKind values handled above
}

static bool parseDirection(
  PyObject* object,
  SNLLogicalCone::Direction& direction) {
  if (PyLong_Check(object)) {
    auto value = PyLong_AsLong(object);
    if (value == static_cast<long>(SNLLogicalCone::Direction::FanIn)) {
      direction = SNLLogicalCone::Direction::FanIn;
      return true;
    }
    if (value == static_cast<long>(SNLLogicalCone::Direction::FanOut)) {
      direction = SNLLogicalCone::Direction::FanOut;
      return true;
    }
  } else if (PyUnicode_Check(object)) {
    std::string value(PyUnicode_AsUTF8(object));
    if (value == "fanin") {
      direction = SNLLogicalCone::Direction::FanIn;
      return true;
    }
    if (value == "fanout") {
      direction = SNLLogicalCone::Direction::FanOut;
      return true;
    }
  }
  return false;
}

static int PySNLLogicalCone_Init(
  PySNLLogicalCone* self,
  PyObject* args,
  PyObject* kwargs) {
  PyObject* startsObject = nullptr;
  PyObject* directionObject = nullptr;
  static const char* keywords[] = {"start", "direction", nullptr};
  if (not PyArg_ParseTupleAndKeywords(
      args,
      kwargs,
      "OO:SNLLogicalCone",
      const_cast<char**>(keywords),
      &startsObject,
      &directionObject)) {
    setError("malformed SNLLogicalCone create method");
    return -1;
  }

  SNLLogicalCone::Direction direction;
  if (not parseDirection(directionObject, direction)) {
    setError("SNLLogicalCone direction must be FanIn or FanOut");
    return -1;
  }

  if (not IsPySNLOccurrence(startsObject)) {
    setError("SNLLogicalCone start must be one SNLOccurrence");
    return -1;
  }
  const auto& start = *PYSNLOccurrence_O(startsObject);
  if (not start.isValid() or
      (not start.getBitTerm() and not start.getInstTerm())) {
    setError(
      "SNLLogicalCone start must be a valid single-bit "
      "SNLNetComponent occurrence");
    return -1;
  }

  try {
    self->object_ = new SNLLogicalCone(start, direction);
  // The wrapper validates every public constructor precondition above.
  // LCOV_EXCL_START
  } catch (const NLException& exception) {
    setError("Naja NL error: " + exception.getReason());
    return -1;
  } catch (const std::exception& exception) {
    setError("Exception " + std::string(exception.what()));
    return -1;
  }
  // LCOV_EXCL_STOP
  return 0;
}

static void PySNLLogicalCone_DeAlloc(PySNLLogicalCone* self) {
  delete self->object_;
  PyObject_DEL(self);
}

static PyObject* PySNLLogicalCone_Repr(PySNLLogicalCone* self) {
  std::ostringstream stream;
  stream << "SNLLogicalCone(";
  if (self->object_) {
    stream << "nodes=" << self->object_->getNodeCount();
  } else {
    stream << "unbound";
  }
  stream << ")";
  return PyUnicode_FromString(stream.str().c_str());
}

static PyObject* nodeIDsToTuple(
  const std::vector<SNLLogicalCone::NodeID>& ids) {
  auto tuple = PyTuple_New(static_cast<Py_ssize_t>(ids.size()));
  if (not tuple) { // LCOV_EXCL_LINE allocation failure
    return nullptr; // LCOV_EXCL_LINE
  }
  for (size_t i = 0; i < ids.size(); ++i) {
    PyTuple_SET_ITEM(
      tuple,
      static_cast<Py_ssize_t>(i),
      PyLong_FromUnsignedLong(ids[i]));
  }
  return tuple;
}

static PyObject* nodeToTuple(
  SNLLogicalCone::NodeID id,
  const SNLLogicalCone::Node& node) {
  auto tuple = PyTuple_New(5);
  // LCOV_EXCL_START
  if (not tuple) {
    return nullptr;
  }
  // LCOV_EXCL_STOP
  PyTuple_SET_ITEM(tuple, 0, PyLong_FromUnsignedLong(id));
  PyTuple_SET_ITEM(tuple, 1, PySNLOccurrence_Link(node.occurrence));
  PyTuple_SET_ITEM(tuple, 2, PyUnicode_FromString(nodeKindString(node.kind)));
  PyTuple_SET_ITEM(tuple, 3, nodeIDsToTuple(node.next));
  PyTuple_SET_ITEM(tuple, 4, nodeIDsToTuple(node.prev));
  return tuple;
}

static PyObject* nodesToTuple(
  const SNLLogicalCone& cone,
  const std::vector<SNLLogicalCone::NodeID>& ids) {
  auto tuple = PyTuple_New(static_cast<Py_ssize_t>(ids.size()));
  // LCOV_EXCL_START
  if (not tuple) {
    return nullptr;
  }
  // LCOV_EXCL_STOP
  const auto& nodes = cone.getNodes();
  for (size_t i = 0; i < ids.size(); ++i) {
    PyTuple_SET_ITEM(
      tuple,
      static_cast<Py_ssize_t>(i),
      nodeToTuple(ids[i], nodes[ids[i]]));
  }
  return tuple;
}

static PyObject* PySNLLogicalCone_getNodes(PySNLLogicalCone* self) {
  if (not self->object_) {
    setError("unbound SNLLogicalCone");
    return nullptr;
  }
  const auto& nodes = self->object_->getNodes();
  auto tuple = PyTuple_New(static_cast<Py_ssize_t>(nodes.size()));
  // LCOV_EXCL_START
  if (not tuple) {
    return nullptr;
  }
  // LCOV_EXCL_STOP
  for (size_t i = 0; i < nodes.size(); ++i) {
    PyTuple_SET_ITEM(
      tuple,
      static_cast<Py_ssize_t>(i),
      nodeToTuple(static_cast<SNLLogicalCone::NodeID>(i), nodes[i]));
  }
  return tuple;
}

static PyObject* PySNLLogicalCone_getRoot(PySNLLogicalCone* self) {
  if (not self->object_) {
    setError("unbound SNLLogicalCone");
    return nullptr;
  }
  auto root = self->object_->getRoot();
  return nodeToTuple(root, self->object_->getNodes()[root]);
}

static PyObject* PySNLLogicalCone_getLeaves(PySNLLogicalCone* self) {
  if (not self->object_) {
    setError("unbound SNLLogicalCone");
    return nullptr;
  }
  return nodesToTuple(*self->object_, self->object_->getLeaves());
}

static PyObject* PySNLLogicalCone_getDirection(PySNLLogicalCone* self) {
  if (not self->object_) {
    setError("unbound SNLLogicalCone");
    return nullptr;
  }
  return toPyLong(self->object_->getDirection());
}

static PyObject* PySNLLogicalCone_getNodeCount(PySNLLogicalCone* self) {
  if (not self->object_) {
    setError("unbound SNLLogicalCone");
    return nullptr;
  }
  return PyLong_FromSize_t(self->object_->getNodeCount());
}

PyMethodDef PySNLLogicalCone_Methods[] = {
  {"get_nodes", (PyCFunction)PySNLLogicalCone_getNodes, METH_NOARGS,
    "Return all DAG nodes as (id, occurrence, kind, next_ids, prev_ids)."},
  {"get_root", (PyCFunction)PySNLLogicalCone_getRoot, METH_NOARGS,
    "Return the root node."},
  {"get_leaves", (PyCFunction)PySNLLogicalCone_getLeaves, METH_NOARGS,
    "Return frontier nodes."},
  {"get_direction", (PyCFunction)PySNLLogicalCone_getDirection, METH_NOARGS,
    "Return the cone direction."},
  {"get_node_count", (PyCFunction)PySNLLogicalCone_getNodeCount, METH_NOARGS,
    "Return the number of DAG nodes."},
  {"getNodes", (PyCFunction)PySNLLogicalCone_getNodes, METH_NOARGS, nullptr},
  {"getRoot", (PyCFunction)PySNLLogicalCone_getRoot, METH_NOARGS, nullptr},
  {"getLeaves", (PyCFunction)PySNLLogicalCone_getLeaves, METH_NOARGS, nullptr},
  {"getDirection", (PyCFunction)PySNLLogicalCone_getDirection, METH_NOARGS, nullptr},
  {"getNodeCount", (PyCFunction)PySNLLogicalCone_getNodeCount, METH_NOARGS, nullptr},
  {nullptr, nullptr, 0, nullptr}
};

// Reserved C++-to-Python link helper; no public wrapper currently calls it.
// LCOV_EXCL_START
PyObject* PySNLLogicalCone_Link(const SNLLogicalCone& logicalCone) {
  auto pyObject = PyObject_NEW(PySNLLogicalCone, &PyTypeSNLLogicalCone);
  if (not pyObject) {
    return nullptr;
  }
  pyObject->object_ = new SNLLogicalCone(logicalCone);
  return reinterpret_cast<PyObject*>(pyObject);
}
// LCOV_EXCL_STOP

void PySNLLogicalCone_LinkPyType() {
  PyTypeSNLLogicalCone.tp_dealloc =
    reinterpret_cast<destructor>(PySNLLogicalCone_DeAlloc);
  PyTypeSNLLogicalCone.tp_repr =
    reinterpret_cast<reprfunc>(PySNLLogicalCone_Repr);
  PyTypeSNLLogicalCone.tp_str =
    reinterpret_cast<reprfunc>(PySNLLogicalCone_Repr);
  PyTypeSNLLogicalCone.tp_init =
    reinterpret_cast<initproc>(PySNLLogicalCone_Init);
  PyTypeSNLLogicalCone.tp_methods = PySNLLogicalCone_Methods;
}

void PySNLLogicalCone_postModuleInit() {
  PyObject* constant;
  LoadObjectConstant(
    PyTypeSNLLogicalCone.tp_dict,
    SNLLogicalCone::Direction::FanIn,
    "FanIn");
  LoadObjectConstant(
    PyTypeSNLLogicalCone.tp_dict,
    SNLLogicalCone::Direction::FanOut,
    "FanOut");
}

PyTypeObjectDefinitions(SNLLogicalCone)

}  // namespace PYNAJA
