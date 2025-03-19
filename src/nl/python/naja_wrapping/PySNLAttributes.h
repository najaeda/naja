// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_ATTRIBUTES_H_
#define __PY_SNL_ATTRIBUTES_H_

#include <Python.h>
#include "NajaCollection.h"

namespace naja::NL {
  class SNLAttribute;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLAttribute>* object_;
} PySNLAttributes;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLAttribute>::Iterator* object_;
  PySNLAttributes* container_;
} PySNLAttributesIterator;

extern PyTypeObject PyTypeSNLAttributes;
extern PyTypeObject PyTypeSNLAttributesIterator;

extern void PySNLAttributes_LinkPyType();

} /* PYNAJA namespace */
 
#endif /* __PY_SNL_ATTRIBUTES_H_ */
