// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_BUS_NET_BIT_H_
#define __PY_SNL_BUS_NET_BIT_H_

#include "PySNLBitNet.h"
#include "NajaPythonExport.h"
#include "SNLBusNetBit.h"

namespace PYNAJA {

typedef struct {
  PySNLBitNet parent_;
} PySNLBusNetBit;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLBusNetBit;

extern PyObject*    PySNLBusNetBit_Link(naja::NL::SNLBusNetBit* n);
extern void         PySNLBusNetBit_LinkPyType();

#define IsPySNLBusNetBit(v)  (PyObject_TypeCheck(v, &PyTypeSNLBusNetBit))
#define PYSNLBusNetBit(v)    (static_cast<PySNLBusNetBit*>(v))
#define PYSNLBusNetBit_O(v)  (PYSNLBusNetBit(v)->object_)

} /* PYNAJA namespace */
 
#endif /* __PY_SNL_BUS_NET_BIT_H_ */
