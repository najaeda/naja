// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLEquipotential.h"
#include "PySNLEquipotentialMode.h"

#include "PyInterface.h"
#include "PySNLNetComponent.h"
#include "PySNLPath.h"
#include "PySNLOccurrence.h"
#include "PySNLOccurrences.h"
#include "PySNLBitTerms.h"

#include "SNLPath.h"
#include "SNLEquipotential.h"
#include "NetlistGraph.h"

namespace PYNAJA {

using namespace naja::NL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLEquipotential, function)

static int PySNLEquipotential_Init(PySNLEquipotential* self, PyObject* args, PyObject* kwargs) {
  SNLEquipotential* equipotential = nullptr;
  PyObject* arg0 = nullptr;
  PyObject* modeObject = nullptr;
  static const char* keywords[] = {"net_component", "mode", nullptr};

  if (not PyArg_ParseTupleAndKeywords(
      args,
      kwargs,
      "O|O:SNLEquipotential",
      const_cast<char**>(keywords),
      &arg0,
      &modeObject)) {
    setError("malformed SNLEquipotential create method");
    return -1;
  }
  auto mode = SNLEquipotential::Mode::Standard;
  if (modeObject) {
    if (not PyLong_Check(modeObject)) {
      setError("SNLEquipotential mode must be an SNLEquipotential.Mode");
      return -1;
    }
    switch (PyLong_AsLong(modeObject)) {
      case static_cast<long>(SNLEquipotential::Mode::Standard):
        break;
      case static_cast<long>(SNLEquipotential::Mode::TraverseAssigns):
        mode = SNLEquipotential::Mode::TraverseAssigns;
        break;
      default:
        setError("invalid SNLEquipotential.Mode value");
        return -1;
    }
  }
  if (IsPySNLOccurrence(arg0)) {
    const auto occurrence = PYSNLOccurrence_O(arg0);
    if (not occurrence->isNetComponentOccurrence()) {
      setError("SNLOccurrence passed to SNLEquipotential constructor is not a SNLNetComponentOccurrence");
      return -1;
    }
    equipotential = new SNLEquipotential(*occurrence, mode);
  } else if (IsPySNLNetComponent(arg0)) {
    equipotential = new SNLEquipotential(PYSNLNetComponent_O(arg0), mode);
  } else {
    setError("SNLEquipotential create accepts SNLNetComponent or SNLNetComponentOccurrence as only argument");
    return -1;
  }
  self->object_ = equipotential;
  return 0;
}

static PyObject* PySNLEquipotential_dumpDotFile(PySNLEquipotential* self, PyObject* args) {
  char* path = NULL; 
  if (not PyArg_ParseTuple(args, "s:SELF_TYPE.METHOD", &path)) {
    setError("dumpDotFile expact a string as argument");
    return nullptr;
  }
  std::filesystem::path outputPath;
  if (path) {
    outputPath = std::filesystem::path(path);
  }
  auto top = NLUniverse::get()->getTopDesign();
  std::string dotFileName(outputPath.string());
  naja::SnlVisualiser snl(top, true, self->object_);
  snl.process();
  snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
  Py_RETURN_NONE;
}

GetContainerMethod(SNLEquipotential, SNLBitTerm*, SNLBitTerms, Terms)
GetContainerMethod(SNLEquipotential, SNLOccurrence, SNLOccurrences, InstTermOccurrences)
GetBoolAttribute(SNLEquipotential, isConst0)
GetBoolAttribute(SNLEquipotential, isConst1)
GetBoolAttribute(SNLEquipotential, isConstX)
GetBoolAttribute(SNLEquipotential, isConstZ)

//LCOV_EXCL_START
ManagedTypeLinkCreateMethod(SNLEquipotential) 
//LCOV_EXCL_STOP
ManagedTypeDeallocMethod(SNLEquipotential)

PyMethodDef PySNLEquipotential_Methods[] = {
  {  "getTerms", (PyCFunction)PySNLEquipotential_getTerms, METH_NOARGS,
     "Returns the equi top terms."},
  {  "getInstTermOccurrences", (PyCFunction)PySNLEquipotential_getInstTermOccurrences, METH_NOARGS,
     "Returns the equi inst terms."},
  {  "isConst0", (PyCFunction)PySNLEquipotential_isConst0, METH_NOARGS,
     "Returns True if the equipotential is constant 0."},
  {  "isConst1", (PyCFunction)PySNLEquipotential_isConst1, METH_NOARGS,
     "Returns True if the equipotential is constant 1."},
  {  "isConstX", (PyCFunction)PySNLEquipotential_isConstX, METH_NOARGS,
     "Returns True if the equipotential is constant X."},
  {  "isConstZ", (PyCFunction)PySNLEquipotential_isConstZ, METH_NOARGS,
     "Returns True if the equipotential is constant Z."},
  {  "dumpDotFile", (PyCFunction)PySNLEquipotential_dumpDotFile, METH_VARARGS,
     "Dump the dot file."},
  {NULL, NULL, 0, NULL} /* sentinel */
};

PyTypeManagedNLObjectWithoutNLIDLinkPyType(SNLEquipotential)
PyTypeObjectDefinitions(SNLEquipotential)

void PySNLEquipotential_postModuleInit() {
  PySNLEquipotentialMode_postModuleInit();
  PyDict_SetItemString(
    PyTypeSNLEquipotential.tp_dict,
    "Mode",
    reinterpret_cast<PyObject*>(&PyTypeSNLEquipotentialMode));
}

}  // namespace PYNAJA
