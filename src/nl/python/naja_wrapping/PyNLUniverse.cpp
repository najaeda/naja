// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PyNLUniverse.h"

#include "PyInterface.h"
#include "PyNLDB.h"
#include "PyNLDBs.h"
#include "PySNLDesign.h"

#include "NLUniverse.h"
#include "RemoveLoadlessLogic.h"
#include "ConstantPropagation.h"
#include "FanoutComputer.h"
#include "LogicLevelComputer.h"
#include "SNLNetComponentOccurrence.h"
#include "PySNLNetComponentOccurrence.h"

namespace PYNAJA {

using namespace naja::NL;
using namespace naja::NAJA_OPT;
using namespace naja::NAJA_METRICS;


#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(NLUniverse, function)

static PyObject* PyNLUniverse_create() {
  NLUniverse* universe = nullptr;
  TRY
  universe = NLUniverse::create();
  NLCATCH
  return PyNLUniverse_Link(universe);
}

static PyObject* PyNLUniverse_get() {
  auto universe = NLUniverse::get();
  return PyNLUniverse_Link(universe);
}

static PyObject* PyNLUniverse_applyDLE() {
 LoadlessLogicRemover remover;
 remover.setNormalizedUniquification(true);
 remover.process();
 Py_RETURN_NONE;
}

// fanout calculation
static PyObject* PyNLUniverse_getMaxFanout() {
  FanoutComputer fanoutComputer;
  fanoutComputer.process();
  const auto& fanouts = fanoutComputer.getMaxFanoutTerms();
  std::vector<std::pair<SNLNetComponentOccurrence, std::vector<SNLNetComponentOccurrence>>> pyFanouts;
  for (const auto& [termID, readerIDs] : fanouts) {
    const DNLTerminalFull& term = naja::DNL::get()->getDNLTerminalFromID(termID);
    SNLNetComponentOccurrence pyTerm = term.getOccurrence();
    std::vector<SNLNetComponentOccurrence> pyReaders;
    for (const auto& readerID : readerIDs) {
      const DNLTerminalFull& readerTerm = naja::DNL::get()->getDNLTerminalFromID(readerID);
      pyReaders.push_back(readerTerm.getOccurrence());
    }
    pyFanouts.push_back({pyTerm, pyReaders});
  }
  // return a pair (maxFanout, terms(with PySNLNetComponentOccurrence))
  PyObject* py_list = PyList_New(2);
  PyList_SetItem(py_list, 0, PyLong_FromSize_t(fanoutComputer.getMaxFanout()));
  PyObject* py_terms_list = PyList_New(pyFanouts.size());
  for (size_t i = 0; i < pyFanouts.size(); i++) {
    PyObject* readers_list = PyList_New(pyFanouts[i].second.size());
    for (size_t j = 0; j < pyFanouts[i].second.size(); j++) {
      PyList_SetItem(readers_list, j, PySNLNetComponentOccurrence_Link(pyFanouts[i].second[j]));
    }
    PyObject* term_and_readers = PyTuple_New(2);
    PyTuple_SetItem(term_and_readers, 0, PySNLNetComponentOccurrence_Link(pyFanouts[i].first));
    PyTuple_SetItem(term_and_readers, 1, readers_list);
    PyList_SetItem(py_terms_list, i, term_and_readers);
  }
  PyList_SetItem(py_list, 1, py_terms_list);
  return py_list;
}

static PyObject* PyNLUniverse_getMaxLogicLevel() {
  LogicLevelComputer logicLevelComputer;
  logicLevelComputer.process();
  const std::vector<std::vector<std::pair<DNLID, DNLID>>>& paths = logicLevelComputer.getMaxLogicLevelPaths();
  std::vector<std::vector<SNLNetComponentOccurrence>> pyPaths;
  for (const auto& path : paths) {
    std::vector<SNLNetComponentOccurrence> pyPath;
    for (const auto& [fromTermID, toTermID] : path) {
      if (fromTermID != naja::DNL::DNLID_MAX) {
        const DNLTerminalFull& fromTerm = naja::DNL::get()->getDNLTerminalFromID(fromTermID);
        // only one term in the path
        pyPath.push_back(fromTerm.getOccurrence());
      }
      if (toTermID != naja::DNL::DNLID_MAX) {
        const DNLTerminalFull& toTerm = naja::DNL::get()->getDNLTerminalFromID(toTermID);
        pyPath.push_back(toTerm.getOccurrence());
      }
    }
    pyPaths.push_back(pyPath);
  }
  // return a pair (maxLogicLevel, paths(with PySNLNetComponentOccurrence))
  PyObject* py_list = PyList_New(2);
  PyList_SetItem(py_list, 0, PyLong_FromSize_t(logicLevelComputer.getMaxLogicLevel()));
  PyObject* py_paths_list = PyList_New(pyPaths.size());
  for (size_t i = 0; i < pyPaths.size(); i++) {
    PyObject* path_list = PyList_New(pyPaths[i].size());
    for (size_t j = 0; j < pyPaths[i].size(); j++) {
      PyList_SetItem(path_list, j, PySNLNetComponentOccurrence_Link(pyPaths[i][j]));
    }
    PyList_SetItem(py_paths_list, i, path_list);
  }
  PyList_SetItem(py_list, 1, py_paths_list);
  return py_list;
}

static PyObject* PyNLUniverse_applyConstantPropagation() {
  ConstantPropagation cp;
  cp.setTruthTableEngine(true);
  cp.run();
  Py_RETURN_NONE;
}


static PyObject* PyNLUniverse_setTopDesign(PyNLUniverse* self, PyObject* arg) {
  METHOD_HEAD("NLUniverse.setTopDesign()")
  if (IsPySNLDesign(arg)) {
    selfObject->setTopDesign(PYSNLDesign_O(arg));
  } else {
    setError("NLUniverse setTopDesign takes SNLDesign argument");
    return nullptr;
  }
  Py_RETURN_NONE;
}

static PyObject* PyNLUniverse_setTopDB(PyNLUniverse* self, PyObject* arg) {
  METHOD_HEAD("NLUniverse.setTopDB()")
  if (IsPyNLDB(arg)) {
    selfObject->setTopDB(PYNLDB_O(arg));
  } else {
    setError("NLUniverse setTopDB takes SNLDesign argument");
    return nullptr;
  }
  Py_RETURN_NONE;
}

GetObjectMethod(NLUniverse, SNLDesign, getTopDesign)
GetObjectMethod(NLUniverse, NLDB, getTopDB)
GetObjectByIndex(NLUniverse, NLDB, DB)
GetContainerMethod(NLUniverse, NLDB*, NLDBs, UserDBs)

DBoDestroyAttribute(PyNLUniverse_destroy, PyNLUniverse)

PyMethodDef PyNLUniverse_Methods[] = {
  { "create", (PyCFunction)PyNLUniverse_create, METH_NOARGS|METH_STATIC,
    "create the NLUniverse (static object)"},
  { "destroy", (PyCFunction)PyNLUniverse_destroy, METH_NOARGS,
    "destroy the associated NLUniverse"},
  { "get", (PyCFunction)PyNLUniverse_get, METH_NOARGS|METH_STATIC,
    "get the NLUniverse (static object)"},
  { "getTopDesign", (PyCFunction)PyNLUniverse_getTopDesign, METH_NOARGS,
    "get the top SNLDesign"},
  { "setTopDesign", (PyCFunction)PyNLUniverse_setTopDesign, METH_O,
    "set the top SNLDesign"},
  { "setTopDB", (PyCFunction)PyNLUniverse_setTopDB, METH_O,
    "set the top NLDB"},
  { "getTopDB", (PyCFunction)PyNLUniverse_getTopDB, METH_NOARGS,
    "get the Top NLDB"},
  { "getDB", (PyCFunction)PyNLUniverse_getDB, METH_VARARGS,
    "get the NLDB with the given index"},
  { "getUserDBs", (PyCFunction)PyNLUniverse_getUserDBs, METH_NOARGS,
    "iterate on User NLDBs."},
  { "applyDLE", (PyCFunction)PyNLUniverse_applyDLE, METH_NOARGS|METH_STATIC,
   "apply Dead Logic Elimination on the top design of the NLUniverse."},
  { "applyConstantPropagation", (PyCFunction)PyNLUniverse_applyConstantPropagation, METH_NOARGS|METH_STATIC,
    "apply Constant Propagation on the top design of the NLUniverse."},
  { "getMaxFanout", (PyCFunction)PyNLUniverse_getMaxFanout, METH_NOARGS|METH_STATIC,
    "get the maximum fanout of the top design of the NLUniverse."},
  { "getMaxLogicLevel", (PyCFunction)PyNLUniverse_getMaxLogicLevel, METH_NOARGS|METH_STATIC,
    "get the maximum logic level of the top design of the NLUniverse."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(NLUniverse)

DBoLinkCreateMethod(NLUniverse)
PyTypeNLObjectWithoutNLIDLinkPyType(NLUniverse)
PyTypeObjectDefinitions(NLUniverse)

}