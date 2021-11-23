#include "PySNLNetComponent.h"

#include "PySNLDesign.h"

namespace PYSNL {

using namespace SNL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT           parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function)   GENERIC_METHOD_HEAD(NetComponent, netComponent, function)

PyMethodDef PySNLNetComponent_Methods[] = {
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDestroyAttribute(PySNLNetComponent_destroy, PySNLNetComponent)
DBoDeallocMethod(SNLNetComponent)

DBoLinkCreateMethod(SNLNetComponent)
PyTypeObjectLinkPyType(SNLNetComponent)
PyTypeInheritedObjectDefinitions(SNLNetComponent, SNLDesignObject)

}
