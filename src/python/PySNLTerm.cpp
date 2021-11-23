#include "PySNLTerm.h"

#include "PySNLDesign.h"

namespace PYSNL {

using namespace SNL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT            parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function)    GENERIC_METHOD_HEAD(Term, term, function)

PyMethodDef PySNLTerm_Methods[] = {
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDestroyAttribute(PySNLTerm_destroy, PySNLTerm)
DBoDeallocMethod(SNLTerm)

DBoLinkCreateMethod(SNLTerm)
PyTypeObjectLinkPyType(SNLTerm)
PyTypeInheritedObjectDefinitions(SNLTerm, SNLNetComponent)

}
