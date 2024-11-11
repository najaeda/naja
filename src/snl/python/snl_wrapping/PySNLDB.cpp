// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLDB.h"

#include "PyInterface.h"
#include "PySNLUniverse.h"
#include "PySNLLibrary.h"
#include "PySNLLibraries.h"

#include "SNLDB.h"

#include <filesystem>
#include "SNLCapnP.h"
#include <Python.h>

namespace PYSNL
{

  using namespace naja::SNL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLDB, function)

  static PyObject *PySNLDB_create(PyObject *, PyObject *args)
  {
    PyObject *arg = nullptr;
    if (not PyArg_ParseTuple(args, "O:SNLDB.create", &arg))
    {
      setError("malformed SNLDB create");
      return nullptr;
    }
    if (not IsPySNLUniverse(arg))
    {
      setError("SNLDB create argument should be a SNLUniverse");
      return nullptr;
    }
    auto universe = PYSNLUNIVERSE_O(arg);
    SNLDB *db = nullptr;
    TRY
        db = SNLDB::create(universe);
    SNLCATCH
    return PySNLDB_Link(db);
  }

  static PyObject *PySNLDB_loadSNL(PyObject *, PyObject *args)
  {
    PyObject *arg = nullptr;
    if (not PyArg_ParseTuple(args, "O:SNLDB.loadSNL", &arg))
    {
      setError("malformed SNLDB loadSNL");
      return nullptr;
    }
    if (not PyUnicode_Check(arg))
    {
      setError("SNLDB loadSNL argument should be a file path");
      return nullptr;
    }
    SNLDB *db = nullptr;
    const std::filesystem::path path(PyUnicode_AsUTF8(arg));
    if (SNLUniverse::get() == nullptr)
    {
      SNLUniverse::create();
    }
    TRY
        db = SNLCapnP::load(path);
    // SNLUniverse::get()->setTopDesign(db->getTopDesign());
    SNLCATCH
    return PySNLDB_Link(db);
  }

  PyObject *PySNLDB_dumpSNL(PySNLDB *self, PyObject *args)
  {
    PyObject *arg = nullptr;
    if (not PyArg_ParseTuple(args, "O:SNLDB.dumpSNL", &arg))
    {
      setError("malformed SNLDB dumpSNL");
      Py_RETURN_FALSE;
    }
    if (not PyUnicode_Check(arg))
    {
      setError("SNLDB dumpSNL argument should be a file path");
      Py_RETURN_FALSE;
    }
    SNLCapnP::dump(self->object_, PyUnicode_AsUTF8(arg));
    // return true to python
    Py_RETURN_TRUE;
  }

  GetObjectByName(SNLDB, SNLLibrary, getLibrary)
      GetContainerMethod(DB, Library, Libraries, Libraries)

          DBoDestroyAttribute(PySNLDB_destroy, PySNLDB)

              PyMethodDef PySNLDB_Methods[] = {
                  {"create", (PyCFunction)PySNLDB_create, METH_VARARGS | METH_STATIC,
                   "create a SNLDB."},
                  {"loadSNL", (PyCFunction)PySNLDB_loadSNL, METH_VARARGS | METH_STATIC,
                   "create a SNLDB from SNL format."},
                  {"dumpSNL", (PyCFunction)PySNLDB_dumpSNL, METH_VARARGS,
                   "dump this SNLDB to SNL format."},
                  {"getLibrary", (PyCFunction)PySNLDB_getLibrary, METH_VARARGS,
                   "retrieve a SNLLibrary."},
                  {"getLibraries", (PyCFunction)PySNLDB_getLibraries, METH_NOARGS,
                   "get a container of SNLLibraries."},
                  {"destroy", (PyCFunction)PySNLDB_destroy, METH_NOARGS,
                   "destroy this SNLDB."},
                  {NULL, NULL, 0, NULL} /* sentinel */
  };

  DBoDeallocMethod(SNLDB)

      DBoLinkCreateMethod(SNLDB)
          PyTypeSNLFinalObjectWithSNLIDLinkPyType(SNLDB)
              PyTypeObjectDefinitions(SNLDB)

}
