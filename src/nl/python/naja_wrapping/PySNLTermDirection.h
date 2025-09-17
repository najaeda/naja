// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_TERM_DIRECTION_H_
#define __PY_SNL_TERM_DIRECTION_H_

#include "PyInterface.h"
#include "SNLTerm.h"

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NL::SNLTerm::Direction* object_;
} PySNLTermDirection;

extern PyTypeObject PyTypeSNLTermDirection;

extern PyObject*    PySNLTermDirection_Link(naja::NL::SNLTerm::Direction*);
extern void         PySNLTermDirection_LinkPyType();
extern void         PySNLTermDirection_postModuleInit();


#define IsPySNLTermDirection(v) (PyObject_TypeCheck(v, &PyTypeSNLTermDirection))
#define PYSNLTermDirection(v)   (static_cast<PySNLTermDirection*>(v))
#define PYSNLTermDirection_O(v) (static_cast<naja::NL::SNLTerm::Direction*>(PYSNLTermDirection(v)->object_))

} /* PYNAJA namespace */

#endif /* __PY_SNL_TERM_DIRECTION_H_ */
