#ifndef __PY_INTERFACE_H
#define __PY_INTERFACE_H

#include <iostream>
#include <sstream>

#define PY_SSIZE_T_CLEAN /* Make "s#" use Py_ssize_t rather than int. */
#include <Python.h>

// This macro must be redefined in derived classes.
// Example : baseOject_.object_
#define ACCESS_OBJECT object_

#define DirectReprMethod(PY_FUNC_NAME,PY_SELF_TYPE,SELF_TYPE)                                   \
  static PyObject* PY_FUNC_NAME(PY_SELF_TYPE *self ) {                                          \
    if (not self->ACCESS_OBJECT) {                                                              \
      std::ostringstream repr;                                                                  \
      repr << "<" #PY_SELF_TYPE " [" << (void*)self << " <-> nullptr] unbound>";                \
      return PyUnicode_FromString(repr.str().c_str());                                          \
    }                                                                                           \
    SELF_TYPE* object = dynamic_cast<SELF_TYPE*>(self->ACCESS_OBJECT);                          \
    if (not object) {                                                                           \
      return PyUnicode_FromString("<PyObject invalid dynamic_cast>");                           \
    }                                                                                           \
    std::ostringstream repr;                                                                    \
    repr << "[" << (void*)self << "<->" << (void*)object << " " << object->getString() << "]";  \
    return PyUnicode_FromString(repr.str().c_str() );                                           \
  }

#define DirectStrMethod(PY_FUNC_NAME,PY_SELF_TYPE,SELF_TYPE)                      \
  static PyObject* PY_FUNC_NAME(PY_SELF_TYPE *self) {                             \
    if (not self->ACCESS_OBJECT) {                                                \
      std::ostringstream repr;                                                    \
      repr << "<" #PY_SELF_TYPE " [" << (void*)self << " <-> nullptr] unbound>";  \
      return PyUnicode_FromString(repr.str().c_str());                            \
    }                                                                             \
    SELF_TYPE* object = dynamic_cast<SELF_TYPE*>(self->ACCESS_OBJECT);            \
    if (not object)                                                               \
      return PyUnicode_FromString("<PyObject invalid dynamic_cast>" );            \
                                                                                  \
    return PyUnicode_FromString(object->getString().c_str() );                    \
  }

#define DirectCmpMethod(PY_FUNC_NAME,IS_PY_OBJECT,PY_SELF_TYPE)               \
  static int PY_FUNC_NAME(PY_SELF_TYPE *self, PyObject* other) {              \
    if (!IS_PY_OBJECT(other)) { return -1; }                                  \
                                                                              \
    PY_SELF_TYPE* otherPyObject = (PY_SELF_TYPE*)other;                       \
    if (self->ACCESS_OBJECT == otherPyObject->ACCESS_OBJECT) return 0;        \
    if (self->ACCESS_OBJECT < otherPyObject->ACCESS_OBJECT) return -1;        \
                                                                              \
    return 1;                                                                 \
  }

#define DirectHashMethod(PY_FUNC_NAME,PY_SELF_TYPE)                           \
  static int PY_FUNC_NAME (PY_SELF_TYPE *self) {                              \
    return (long)self->ACCESS_OBJECT;                                         \
  }

#define DBoDestroyAttribute(PY_FUNC_NAME, PY_SELF_TYPE)                                    \
  static PyObject* PY_FUNC_NAME(PY_SELF_TYPE *self) {                                      \
    if (not self->ACCESS_OBJECT) {                                                         \
      std::ostringstream message;                                                          \
      message << "applying a destroy() to a Python object with no object attached";        \
      /*PyErr_SetString(ProxyError, message.str().c_str());*/                              \
      return nullptr;                                                                      \
    }                                                                                      \
    self->ACCESS_OBJECT->destroy();                                                        \
    self->ACCESS_OBJECT = nullptr;                                                         \
    Py_RETURN_NONE;                                                                        \
  }

#define DBoDeallocMethod(SELF_TYPE)                                      \
  static void Py##SELF_TYPE##_DeAlloc(Py##SELF_TYPE *self) {             \
    PyObject_DEL(self);                                                  \
  }

#define PyTypeObjectLinkPyTypeWithClass(PY_SELF_TYPE,SELF_TYPE)                   \
  DirectReprMethod(Py##PY_SELF_TYPE##_Repr, Py##PY_SELF_TYPE,   SELF_TYPE)        \
  DirectStrMethod (Py##PY_SELF_TYPE##_Str,  Py##PY_SELF_TYPE,   SELF_TYPE)        \
  DirectCmpMethod (Py##PY_SELF_TYPE##_Cmp,  IsPy##PY_SELF_TYPE, Py##PY_SELF_TYPE) \
  DirectHashMethod(Py##PY_SELF_TYPE##_Hash, Py##SELF_TYPE)                        \
  extern void  Py##PY_SELF_TYPE##_LinkPyType() {                                  \
    PyType##PY_SELF_TYPE.tp_dealloc = (destructor) Py##PY_SELF_TYPE##_DeAlloc;    \
    /*PyType##PY_SELF_TYPE.tp_compare = (cmpfunc)    Py##PY_SELF_TYPE##_Cmp;*/    \
    PyType##PY_SELF_TYPE.tp_repr    = (reprfunc)   Py##PY_SELF_TYPE##_Repr;       \
    PyType##PY_SELF_TYPE.tp_str     = (reprfunc)   Py##PY_SELF_TYPE##_Str;        \
    PyType##PY_SELF_TYPE.tp_hash    = (hashfunc)   Py##PY_SELF_TYPE##_Hash;       \
    PyType##PY_SELF_TYPE.tp_methods = Py##PY_SELF_TYPE##_Methods;                 \
  }

#define DBoLinkCreateMethod(SELF_TYPE)                                         \
  PyObject* Py##SELF_TYPE##_Link(SELF_TYPE* object) {                          \
    if (not object) {                                                          \
      Py_RETURN_NONE;                                                          \
    }                                                                          \
    Py##SELF_TYPE* pyObject = PyObject_NEW(Py##SELF_TYPE, &PyType##SELF_TYPE); \
    pyObject->ACCESS_OBJECT = object;                                          \
    return (PyObject*)pyObject;                                                \
  }

#define PyTypeObjectLinkPyType(SELF_TYPE) \
  PyTypeObjectLinkPyTypeWithClass(SELF_TYPE,SELF_TYPE)

#define PYTYPE_READY(TYPE)                                        \
  if (PyType_Ready(&PyType##TYPE) < 0) {                          \
    std::cerr << "[ERROR] Failed to initialize <Py" #TYPE ">."    \
      << std::endl;                                               \
    return nullptr;                                               \
  }

#define  PYTYPE_READY_SUB(TYPE, TYPE_BASE)                        \
  PyType##TYPE.tp_base = &PyType##TYPE_BASE;                      \
  if (PyType_Ready(&PyType##TYPE) < 0) {                          \
    std::cerr << "[ERROR]\n"                                      \
         << "  Failed to initialize <Py" #TYPE ">." << std::endl; \
    return nullptr;                                               \
  }

#define PyTypeObjectDefinitions(SELF_TYPE)                        \
  PyTypeObject PyType##SELF_TYPE = {                              \
    PyVarObject_HEAD_INIT(nullptr, 0)                             \
    .tp_name = "#SELF_TYPE",                                      \
    .tp_doc = "#SELF_TYPE objects",                               \
    .tp_basicsize = sizeof(Py##SELF_TYPE),                        \
    .tp_itemsize = 0,                                             \
    .tp_flags = Py_TPFLAGS_DEFAULT,                               \
    .tp_new = PyType_GenericNew,                                  \
  };
 
#endif /* __PY_INTERFACE_H */
