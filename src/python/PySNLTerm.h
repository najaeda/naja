#ifndef __PY_SNL_TERM_H_
#define __PY_SNL_TERM_H_

#include "PySNLNetComponent.h"
#include "SNLTerm.h"

namespace PYSNL {

typedef struct {
  PySNLNetComponent parent_;
} PySNLTerm;

extern PyTypeObject PyTypeSNLTerm;
extern PyMethodDef  PySNLTerm_Methods[];

extern PyObject*    PySNLTerm_Link(SNL::SNLTerm*);
extern void         PySNLTerm_LinkPyType();

#define IsPySNLTerm(v) ((v)->ob_type == &PyTypeSNLTerm)
#define PYSNLTerm(v)   (static_cast<PySNLTerm*>(v))
#define PYSNLTerm_O(v) (static_cast<SNL::SNLTerm*>(PYSNLTerm(v)->parent_->parent_->object_))

} /* PYSNL namespace */
 
#endif /* __PY_SNL_TERM_H_ */
