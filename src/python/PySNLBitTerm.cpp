#include "PySNLTerm.h"

#include "PySNLBitTerm.h"

namespace PYSNL {

using namespace SNL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT            parent_.parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function)    GENERIC_METHOD_HEAD(BitTerm, term, function)

PyMethodDef PySNLBitTerm_Methods[] = {
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDestroyAttribute(PySNLBitTerm_destroy, PySNLBitTerm)
DBoDeallocMethod(SNLBitTerm)

DBoLinkCreateMethod(SNLBitTerm)
PyTypeObjectLinkPyType(SNLBitTerm)
PyTypeInheritedObjectDefinitions(SNLBitTerm, SNLTerm)

}
