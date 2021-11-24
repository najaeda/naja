#ifndef __PY_SNL_DB_H_
#define __PY_SNL_DB_H_

#include "PyInterface.h"
#include "SNLDB.h"

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  SNL::SNLDB* object_;
} PySNLDB;

extern PyTypeObject PyTypeSNLDB;
extern PyMethodDef  PySNLDB_Methods[];

extern PyObject*    PySNLDB_Link(SNL::SNLDB* u);
extern void         PySNLDB_LinkPyType();

#define IsPySNLDB(v) (PyObject_TypeCheck(v, &PyTypeSNLDB))
#define PYSNLDB(v)   ((PySNLDB*)(v))
#define PYSNLDB_O(v) (PYSNLDB(v)->object_)

} /* PYSNL namespace */
 
#endif /* __PY_SNL_DB_H_ */
