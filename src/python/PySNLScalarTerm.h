#ifndef __PY_SNL_SCALAR_TERM_H_
#define __PY_SNL_SCALAR_TERM_H_

#include "PySNLBitTerm.h"
#include "SNLScalarTerm.h"

namespace PYSNL {

typedef struct {
  PySNLBitTerm parent_;
} PySNLScalarTerm;

extern PyTypeObject PyTypeSNLScalarTerm;
extern PyMethodDef  PySNLScalarTerm_Methods[];

extern PyObject*    PySNLScalarTerm_Link(SNL::SNLScalarTerm* t);
extern void         PySNLScalarTerm_LinkPyType();

#define IsPySNLScalarTerm(v) (PyObject_TypeCheck(v, &PyTypeSNLScalarTerm))
#define PYSNLScalarTerm(v)   (static_cast<PySNLScalarTerm*>(v))
#define PYSNLScalarTerm_O(v) (PYSNLScalarTerm(v)->object_)

} /* PYSNL namespace */
 
#endif /* __PY_SNL_SCALAR_TERM_H_ */
