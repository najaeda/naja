#ifndef __PY_SNL_LIBRARY_H_
#define __PY_SNL_LIBRARY_H_

#include "PyInterface.h"
#include "SNLLibrary.h"

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  SNL::SNLLibrary* object_;
} PySNLLibrary;

extern PyTypeObject PyTypeSNLLibrary;
extern PyMethodDef  PySNLLibrary_Methods[];

extern PyObject*    PySNLLibrary_Link(SNL::SNLLibrary* u);
extern void         PySNLLibrary_LinkPyType();


#define IsPySNLLibrary(v) ((v)->ob_type == &PyTypeSNLLibrary)
#define PYSNLLibrary(v)   ((PySNLLibrary*)(v))
#define PYSNLLibrary_O(v) (PYSNLLibrary(v)->object_)

} /* PYSNL namespace */
 
#endif /* __PY_SNL_LIBRARY_H_ */
