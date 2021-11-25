#ifndef __PY_SNL_BIT_NET_H_
#define __PY_SNL_BIT_NET_H_

#include "PySNLNet.h"
#include "SNLBitNet.h"

namespace PYSNL {

typedef struct {
  PySNLNet parent_;
} PySNLBitNet;

extern PyTypeObject PyTypeSNLBitNet;
extern PyMethodDef  PySNLBitNet_Methods[];

extern PyObject*    PySNLBitNet_Link(SNL::SNLBitNet*);
extern void         PySNLBitNet_LinkPyType();

#define IsPySNLBitNet(v) (PyObject_TypeCheck(v, &PyTypeSNLBitNet))
#define PYSNLBitNet(v)   ((PySNLBitNet*)(v))
#define PYSNLBitNet_O(v) (static_cast<SNL::SNLBitNet*>(PYSNLBitNet(v)->parent_.parent_.object_))

} /* PYSNL namespace */
 
#endif /* __PY_SNL_BIT_NET_H_ */
