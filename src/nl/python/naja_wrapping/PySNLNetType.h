// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_NET_TYPE_H_
#define __PY_SNL_NET_TYPE_H_

#include "PyInterface.h"
#include "SNLNet.h"

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::SNL::SNLNet::Type* object_;
} PySNLNetType;

extern PyTypeObject PyTypeSNLNetType;

extern PyObject*    PySNLNetType_Link(naja::SNL::SNLNet::Type*);
extern void         PySNLNetType_LinkPyType();
extern void         PySNLNetType_postModuleInit();

#define IsPySNLNetType(v) (PyObject_TypeCheck(v, &PyTypeSNLNetType))
#define PYSNLNetType(v)   (static_cast<PySNLNetType*>(v))
#define PYSNLNetType_O(v) (static_cast<naja::SNL::SNLNet::Type*>(PYSNLNetType(v)->object_))

} /* PYSNL namespace */

#endif /* __PY_SNL_NET_TYPE_H_ */