#ifndef __PY_SNL_SCALAR_NET_H_
#define __PY_SNL_SCALAR_NET_H_

#include "PySNLBitNet.h"
#include "SNLScalarNet.h"

namespace PYSNL {

typedef struct {
  PySNLBitNet parent_;
} PySNLScalarNet;

extern PyTypeObject PyTypeSNLScalarNet;
extern PyMethodDef  PySNLScalarNet_Methods[];

extern PyObject*    PySNLScalarNet_Link(SNL::SNLScalarNet* n);
extern void         PySNLScalarNet_LinkPyType();

#define IsPySNLScalarNet(v) (PyObject_TypeCheck(v, &PyTypeSNLScalarNet))
#define PYSNLScalarNet(v)   (static_cast<PySNLScalarNet*>(v))
#define PYSNLScalarNet_O(v) (PYSNLScalarNet(v)->object_)

} /* PYSNL namespace */
 
#endif /* __PY_SNL_SCALAR_NET_H_ */
