#include "PySNLUniverse.h"
#include "PySNLDB.h"
#include "PySNLLibrary.h"
#include "PySNLDesign.h"
#include "PySNLScalarTerm.h"
#include "PySNLInstance.h"

namespace PYSNL {

static PyMethodDef SNLMethods[] = {
  {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef snlModule = {
  PyModuleDef_HEAD_INIT,
  "snl",
  "Python interface for SNL netlist API",
  -1,
  SNLMethods
};

PyMODINIT_FUNC PyInit_snl(void) {
  PySNLUniverse_LinkPyType();
  PySNLDB_LinkPyType();
  PySNLLibrary_LinkPyType();
  PySNLDesign_LinkPyType();
  PySNLScalarTerm_LinkPyType();
  PySNLInstance_LinkPyType();

  PYTYPE_READY(SNLUniverse);
  PYTYPE_READY(SNLDB);
  PYTYPE_READY(SNLLibrary);
  PYTYPE_READY(SNLDesign);
  PYTYPE_READY(SNLScalarTerm);
  PYTYPE_READY(SNLInstance);

  Py_INCREF(&PyTypeSNLUniverse);
  Py_INCREF(&PyTypeSNLDB);
  Py_INCREF(&PyTypeSNLLibrary);
  Py_INCREF(&PyTypeSNLDesign);
  Py_INCREF(&PyTypeSNLScalarTerm);
  Py_INCREF(&PyTypeSNLInstance);

  PyObject* mod = PyModule_Create(&snlModule);

  if (not mod) {
    std::cerr << "[ERROR]\n"
      << "  Failed to initialize SNL python module." << std::endl;
    return nullptr;
  }

  PyModule_AddObject(mod, "SNLUniverse", (PyObject*)&PyTypeSNLUniverse);
  PyModule_AddObject(mod, "SNLDB", (PyObject*)&PyTypeSNLDB);
  PyModule_AddObject(mod, "SNLLibrary", (PyObject*)&PyTypeSNLLibrary);
  PyModule_AddObject(mod, "SNLDesign", (PyObject*)&PyTypeSNLDesign);
  PyModule_AddObject(mod, "SNLScalarTerm", (PyObject*)&PyTypeSNLScalarTerm);
  PyModule_AddObject(mod, "SNLInstance", (PyObject*)&PyTypeSNLInstance);

  return mod;
}

}
