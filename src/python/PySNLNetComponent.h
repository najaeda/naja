#ifndef __PY_SNL_NET_COMPONENT_H_
#define __PY_SNL_NET_COMPONENT_H_

#include "PySNLDesignObject.h"
#include "SNLNetComponent.h"

namespace PYSNL {

typedef struct {
  PySNLDesignObject parent_;
} PySNLNetComponent;

extern PyTypeObject PyTypeSNLNetComponent;
extern PyMethodDef  PySNLNetComponent_Methods[];

extern PyObject*    PySNLNetComponent_Link(SNL::SNLNetComponent* u);
extern void         PySNLNetComponent_LinkPyType();

#define IsPySNLNetComponent(v) ((v)->ob_type == &PyTypeSNLNetComponent)
#define PYSNLNetComponent(v)   (static_cast<PySNLNetComponent*>(v))
#define PYSNLNetComponent_O(v) (static_cast<SNL::SNLNetComponent*>(PYSNLNetComponent(v)->parent_->object_))

} /* PYSNL namespace */
 
#endif /* __PY_SNL_NET_COMPONENT_H_ */
