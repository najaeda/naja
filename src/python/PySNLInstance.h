#ifndef __PY_SNL_INSTANCE_H_
#define __PY_SNL_INSTANCE_H_

#include "PyInterface.h"
#include "SNLInstance.h"

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  SNL::SNLInstance* object_;
} PySNLInstance;

extern PyTypeObject PyTypeSNLInstance;
extern PyMethodDef  PySNLInstance_Methods[];

extern PyObject*    PySNLInstance_Link(SNL::SNLInstance* u);
extern void         PySNLInstance_LinkPyType();


#define IsPySNLInstance(v) ((v)->ob_type == &PyTypeSNLInstance)
#define PYSNLInstance(v)   ((PySNLInstance*)(v))
#define PYSNLInstance_O(v) (PYSNLInstance(v)->object_)

} /* PYSNL namespace */
 
#endif /* __PY_SNL_INSTANCE_H_ */
