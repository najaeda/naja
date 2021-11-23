#ifndef __PY_SNL_BIT_TERM_H_
#define __PY_SNL_BIT_TERM_H_

#include "PySNLTerm.h"
#include "SNLBitTerm.h"

namespace PYSNL {

typedef struct {
  PySNLTerm parent_;
} PySNLBitTerm;

extern PyTypeObject PyTypeSNLBitTerm;
extern PyMethodDef  PySNLBitTerm_Methods[];

extern PyObject*    PySNLBitTerm_Link(SNL::SNLBitTerm*);
extern void         PySNLBitTerm_LinkPyType();

#define IsPySNLBitTerm(v) ((v)->ob_type == &PyTypeSNLBitTerm)
#define PYSNLBitTerm(v)   (static_cast<PySNLBitTerm*>(v))
#define PYSNLBitTerm_O(v) (static_cast<SNL::SNLBitTerm*>(PYSNLBitTerm(v)->parent_->parent_->parent_->object_))

} /* PYSNL namespace */
 
#endif /* __PY_SNL_BIT_TERM_H_ */
