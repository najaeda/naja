#ifndef __PY_SNL_INSTANCE_H_
#define __PY_SNL_INSTANCE_H_

#include "PySNLDesignObject.h"

namespace SNL {
  class SNLInstance;
}

namespace PYSNL {

typedef struct {
  PySNLDesignObject parent_;
} PySNLInstance;

extern PyTypeObject PyTypeSNLInstance;
extern PyMethodDef  PySNLInstance_Methods[];

extern PyObject*    PySNLInstance_Link(SNL::SNLInstance* u);
extern void         PySNLInstance_LinkPyType();

#define IsPySNLInstance(v) (PyObject_TypeCheck(v, &PyTypeSNLInstance))
#define PYSNLInstance(v)   (static_cast<PySNLInstance*>(v))
#define PYSNLInstance_O(v) (static_cast<SNL::SNLInstance*>(PYSNLInstance(v)->parent_->parent_->object_))

} /* PYSNL namespace */
 
#endif /* __PY_SNL_INSTANCE_H_ */
