#ifndef __PY_SNL_DESIGN_H_
#define __PY_SNL_DESIGN_H_

#include "PyInterface.h"

namespace SNL {
  class SNLDesign;
}

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  SNL::SNLDesign* object_;
} PySNLDesign;

extern PyTypeObject PyTypeSNLDesign;
extern PyMethodDef  PySNLDesign_Methods[];

extern PyObject*    PySNLDesign_Link(SNL::SNLDesign* u);
extern void         PySNLDesign_LinkPyType();

#define IsPySNLDesign(v) (PyObject_TypeCheck(v, &PyTypeSNLDesign))
#define PYSNLDesign(v)   ((PySNLDesign*)(v))
#define PYSNLDesign_O(v) (PYSNLDesign(v)->object_)

} /* PYSNL namespace */
 
#endif /* __PY_SNL_DESIGN_H_ */
