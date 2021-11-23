#ifndef __PY_SNL_DESIGN_OBJECT_H_
#define __PY_SNL_DESIGN_OBJECT_H_

#include "PyInterface.h"
#include "SNLDesignObject.h"

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  SNL::SNLDesignObject* object_;
} PySNLDesignObject;

extern PyTypeObject PyTypeSNLDesignObject;
extern PyMethodDef  PySNLDesignObject_Methods[];

extern PyObject*    PySNLDesignObject_Link(SNL::SNLDesignObject* u);
extern void         PySNLDesignObject_LinkPyType();

#define IsPySNLDesignObject(v) ((v)->ob_type == &PyTypeSNLDesignObject)
#define PYSNLDesignObject(v)   ((PySNLDesignObject*)(v))
#define PYSNLDesignObject_O(v) (PYSNLDesignObject(v)->object_)

} /* PYSNL namespace */
 
#endif /* __PY_SNL_DESIGN_OBJECT_H_ */
