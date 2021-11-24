#ifndef __PY_SNL_BUS_NET_BIT_H_
#define __PY_SNL_BUS_NET_BIT_H_

#include "PySNLBitNet.h"
#include "SNLBusNetBit.h"

namespace PYSNL {

typedef struct {
  PySNLBitNet parent_;
} PySNLBusNetBit;

extern PyTypeObject PyTypeSNLBusNetBit;
extern PyMethodDef  PySNLBusNetBit_Methods[];

extern PyObject*    PySNLBusNetBit_Link(SNL::SNLBusNetBit* n);
extern void         PySNLBusNetBit_LinkPyType();

#define IsPySNLBusNetBit(v)  (PyObject_TypeCheck(v, &PyTypeSNLBusNetBit))
#define PYSNLBusNetBit(v)    (static_cast<PySNLBusNetBit*>(v))
#define PYSNLBusNetBit_O(v)  (PYSNLBusNetBit(v)->object_)

} /* PYSNL namespace */
 
#endif /* __PY_SNL_BUS_NET_BIT_H_ */
