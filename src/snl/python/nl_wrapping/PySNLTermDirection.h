// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_TERM_DIRECTION_H_
#define __PY_SNL_TERM_DIRECTION_H_

#include "PyInterface.h"
#include "SNLTerm.h"

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::SNL::SNLTerm::Direction* object_;
} PySNLTermDirection;

extern PyTypeObject PyTypeSNLTermDirection;

extern PyObject*    PySNLTermDirection_Link(naja::SNL::SNLTerm::Direction*);
extern void         PySNLTermDirection_LinkPyType();
extern void         PySNLTermDirection_postModuleInit();


#define IsPySNLTermDirection(v) (PyObject_TypeCheck(v, &PyTypeSNLTermDirection))
#define PYSNLTermDirection(v)   (static_cast<PySNLTermDirection*>(v))
#define PYSNLTermDirection_O(v) (static_cast<naja::SNL::SNLTerm::Direction*>(PYSNLTermDirection(v)->object_))

} /* PYSNL namespace */

#endif /* __PY_SNL_TERM_DIRECTION_H_ */
