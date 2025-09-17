// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLEquipotential.h"
#include "PyInterface.h"
#include "SNLNetComponentOccurrence.h"
#include "PySNLNetComponentOccurrence.h"
#include "PySNLPath.h"
#include "PySNLInstTermOccurrences.h"
#include "PySNLBitTerms.h"
#include "SNLNetComponentOccurrence.h"
#include "SNLPath.h"
#include "SNLEquipotential.h"
#include "NetlistGraph.h"

namespace PYNAJA {

using namespace naja::NL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLEquipotential, function)

static int PySNLEquipotential_Init(PySNLEquipotential* self, PyObject* args, PyObject* kwargs) {
  SNLEquipotential* equipotential = nullptr;
  PyObject* arg0 = nullptr;
  PyObject* arg1 = nullptr;

  //SNLEquipotential has three types of constructors:
  if (not PyArg_ParseTuple(args, "|O:SNLEquipotential", &arg0)) {
    setError("malformed SNLEquipotential create method");
    return -1;
  }
  if (arg0 != nullptr) {
    if (IsPySNLNetComponentOccurrence(arg0)) {
      equipotential = new SNLEquipotential(*PYSNLNetComponentOccurrence_O(arg0));
    } else {
      setError("SNLEquipotential create accepts SNLNetComponentOccurrence as only argument");
      return -1;
    }
  }  else {
    setError("invalid number of parameters for Occurrence constructor.");
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
GetContainerMethod(SNLEquipotential, SNLInstTermOccurrence, SNLInstTermOccurrences, InstTermOccurrences)

//LCOV_EXCL_START
ManagedTypeLinkCreateMethod(SNLEquipotential) 
//LCOV_EXCL_STOP
ManagedTypeDeallocMethod(SNLEquipotential)

PyMethodDef PySNLEquipotential_Methods[] = {
  {"getTerms", (PyCFunction)PySNLEquipotential_getTerms, METH_NOARGS,
    "Returns the equi top terms."},
  {"getInstTermOccurrences", (PyCFunction)PySNLEquipotential_getInstTermOccurrences, METH_NOARGS,
    "Returns the equi inst terms."},
  {"dumpDotFile", (PyCFunction)PySNLEquipotential_dumpDotFile, METH_VARARGS,
    "Dump the dot file."},
  {NULL, NULL, 0, NULL} /* sentinel */
};

PyTypeManagedNLObjectWithoutNLIDLinkPyType(SNLEquipotential)
PyTypeObjectDefinitions(SNLEquipotential)

}  // namespace PYNAJA
