#ifndef __PY_SNL_SCALAR_TERM_H_
#define __PY_SNL_SCALAR_TERM_H_

#include "PyInterface.h"
#include "SNLScalarTerm.h"

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  SNL::SNLScalarTerm* object_;
} PySNLScalarTerm;

extern PyTypeObject PyTypeSNLScalarTerm;
extern PyMethodDef  PySNLScalarTerm_Methods[];

extern PyObject*    PySNLScalarTerm_Link(SNL::SNLScalarTerm* t);
extern void         PySNLScalarTerm_LinkPyType();


#define IsPySNLScalarTerm(v) ((v)->ob_type == &PyTypeSNLScalarTerm)
#define PYSNLScalarTerm(v)   ((PySNLScalarTerm*)(v))
#define PYSNLScalarTerm_O(v) (PYSNLScalarTerm(v)->object_)

} /* PYSNL namespace */
 
#endif /* __PY_SNL_SCALAR_TERM_H_ */
