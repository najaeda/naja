#ifndef __PY_SNL_UNIVERSE_H_
#define __PY_SNL_UNIVERSE_H_

#include "PyInterface.h"
#include "SNLUniverse.h"

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  SNL::SNLUniverse* object_;
} PySNLUniverse;

extern PyTypeObject PyTypeSNLUniverse;
extern PyMethodDef  PySNLUniverse_Methods[];

extern PyObject*    PySNLUniverse_Link(SNL::SNLUniverse* u);
extern void         PySNLUniverse_LinkPyType();


#define IsPySNLUniverse(v) ((v)->ob_type == &PyTypeSNLUniverse)
#define PYSNLUNIVERSE(v)   ((PySNLUniverse*)(v))
#define PYSNLUNIVERSE_O(v) (PYSNLUNIVERSE(v)->object_)

} /* PYSNL namespace */
 
#endif /* __PY_SNL_UNIVERSE_H_ */
