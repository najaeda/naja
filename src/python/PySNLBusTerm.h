#ifndef __PY_SNL_BUS_TERM_H_
#define __PY_SNL_BUS_TERM_H_

#include "PySNLTerm.h"

#include "SNLBusTerm.h"

namespace PYSNL {

typedef struct {
  PySNLTerm parent_;
} PySNLBusTerm;

extern PyTypeObject PyTypeSNLBusTerm;
extern PyMethodDef  PySNLBusTerm_Methods[];

extern PyObject*    PySNLBusTerm_Link(SNL::SNLBusTerm* t);
extern void         PySNLBusTerm_LinkPyType();

#define IsPySNLBusTerm(v) (PyObject_TypeCheck(v, &PyTypeSNLBusTerm))
#define PYSNLBusTerm(v)   ((PySNLBusTerm*)(v))
#define PYSNLBusTerm_O(v) (PYSNLBusTerm(v)->object_)

} /* PYSNL namespace */
 
#endif /* __PY_SNL_SCALAR_TERM_H_ */
