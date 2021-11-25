#ifndef __PY_SNL_NET_H_
#define __PY_SNL_NET_H_

#include "PySNLDesignObject.h"
#include "SNLNet.h"

namespace PYSNL {

typedef struct {
  PySNLDesignObject parent_;
} PySNLNet;

extern PyTypeObject PyTypeSNLNet;
extern PyMethodDef  PySNLNet_Methods[];

extern PyObject*    PySNLNet_Link(SNL::SNLNet*);
extern void         PySNLNet_LinkPyType();

#define IsPySNLNet(v) (PyObject_TypeCheck(v, &PyTypeSNLNet))
#define PYSNLNet(v)   (static_cast<PySNLNet*>(v))
#define PYSNLNet_O(v) (static_cast<SNL::SNLNet*>(PYSNLNet(v)->parent_->object_))

} /* PYSNL namespace */
 
#endif /* __PY_SNL_NET_H_ */
