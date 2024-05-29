// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_INTERFACE_H
#define __PY_INTERFACE_H

#include <iostream>
#include <sstream>

#define PY_SSIZE_T_CLEAN /* Make "s#" use Py_ssize_t rather than int. */
#include <Python.h>

#include "SNLException.h"

namespace PYSNL {

static void setError(const std::string& reason) {
  //Mabybe create a custom error type in the future ?
  PyErr_SetString(PyExc_RuntimeError, reason.c_str());
}

template <typename T>
PyObject* richCompare(T left, T right, int op) {
  if ((op == Py_LT) and (left <  right)) Py_RETURN_TRUE;
  if ((op == Py_LE) and (left <= right)) Py_RETURN_TRUE;
  if ((op == Py_EQ) and (left == right)) Py_RETURN_TRUE;
  if ((op == Py_NE) and (left != right)) Py_RETURN_TRUE;
  if ((op == Py_GT) and (left >  right)) Py_RETURN_TRUE;
  if ((op == Py_GE) and (left >= right)) Py_RETURN_TRUE;
  Py_RETURN_FALSE; 
}

}

// This macro must be redefined in derived classes.
// Example : baseOject_.object_
#define ACCESS_OBJECT object_

#define SNLTRY try {

#define SNLCATCH                                        \
  } catch (const naja::SNL::SNLException& e) {                \
    setError("SNL exception: " + e.getReason());        \
    return nullptr;                                     \
  } catch (const std::exception& e) {                   \
    setError("Exception " + std::string(e.what()));     \
    return nullptr;                                     \
  } catch (...) {                                       \
    setError("Unknown exception");                      \
    return nullptr;                                     \
  }

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

#define DirectCmpBySNLIDMethod(PY_FUNC_NAME, PY_SELF_TYPE) \
  static PyObject* PY_FUNC_NAME(PY_SELF_TYPE* self, PyObject* other, int op) { \
    if (not (PyObject_TypeCheck(self, Py_TYPE(other)) or PyObject_TypeCheck(other, Py_TYPE(self)))) Py_RETURN_FALSE; \
    PY_SELF_TYPE* otherPyObject = (PY_SELF_TYPE*)other; \
    auto id = self->ACCESS_OBJECT->getSNLID(); \
    auto otherID = otherPyObject->ACCESS_OBJECT->getSNLID(); \
    return richCompare(id, otherID, op); \
  }

#define DirectCmpByPtrMethod(PY_FUNC_NAME, PY_SELF_TYPE) \
  static PyObject* PY_FUNC_NAME(PY_SELF_TYPE* self, PyObject* other, int op) { \
    if (not (PyObject_TypeCheck(self, Py_TYPE(other)) or PyObject_TypeCheck(other, Py_TYPE(self)))) Py_RETURN_FALSE; \
    PY_SELF_TYPE* otherPyObject = (PY_SELF_TYPE*)other; \
    auto selfObject = self->ACCESS_OBJECT; \
    auto otherObject = otherPyObject->ACCESS_OBJECT; \
    return richCompare(selfObject, otherObject, op); \
  }
     
#define DirectHashMethod(PY_FUNC_NAME,PY_SELF_TYPE)                          \
  static int PY_FUNC_NAME(PY_SELF_TYPE *self) {                              \
    return (long)self->ACCESS_OBJECT;                                        \
  }

#define DirectGetIntMethod(PY_FUNC_NAME, FUNC_NAME, PY_SELF_TYPE, SELF_TYPE) \
  static PyObject* PY_FUNC_NAME(PY_SELF_TYPE* self, PyObject *args) { \
    GENERIC_METHOD_HEAD(SELF_TYPE, #FUNC_NAME"()") \
    return Py_BuildValue("i", selfObject->FUNC_NAME()); \
  }

#define DBoDestroyAttribute(PY_FUNC_NAME, PY_SELF_TYPE)                                    \
  static PyObject* PY_FUNC_NAME(PY_SELF_TYPE *self) {                                      \
    if (not self->ACCESS_OBJECT) {                                                         \
      setError("applying a destroy() to a Python object with no object attached");         \
      return nullptr;                                                                      \
    }                                                                                      \
    self->ACCESS_OBJECT->destroy();                                                        \
    self->ACCESS_OBJECT = nullptr;                                                         \
    Py_RETURN_NONE;                                                                        \
  }

#define DirectDeallocMethod(TYPE) \
  static void Py##TYPE##_DeAlloc(Py##TYPE* self) { \
    if (self->ACCESS_OBJECT) { \
      delete self->ACCESS_OBJECT; \
    } \
    PyObject_DEL(self); \
  }

#define DBoDeallocMethod(SELF_TYPE)                                      \
  static void Py##SELF_TYPE##_DeAlloc(Py##SELF_TYPE *self) {             \
    PyObject_DEL(self);                                                  \
  }

#define PyTypeSNLAbstractObjectWithSNLIDLinkPyType(SELF_TYPE) \
  DirectReprMethod(Py##SELF_TYPE##_Repr, Py##SELF_TYPE, SELF_TYPE) \
  DirectStrMethod(Py##SELF_TYPE##_Str, Py##SELF_TYPE, SELF_TYPE) \
  DirectCmpBySNLIDMethod(Py##SELF_TYPE##_Cmp, Py##SELF_TYPE) \
  DirectHashMethod(Py##SELF_TYPE##_Hash, Py##SELF_TYPE) \
  extern void Py##SELF_TYPE##_LinkPyType() { \
    PyType##SELF_TYPE.tp_richcompare = (richcmpfunc)Py##SELF_TYPE##_Cmp; \
    PyType##SELF_TYPE.tp_repr = (reprfunc)Py##SELF_TYPE##_Repr; \
    PyType##SELF_TYPE.tp_str = (reprfunc)Py##SELF_TYPE##_Str; \
    PyType##SELF_TYPE.tp_hash = (hashfunc)Py##SELF_TYPE##_Hash; \
    PyType##SELF_TYPE.tp_methods = Py##SELF_TYPE##_Methods; \
  }

#define PyTypeSNLFinalObjectWithSNLIDLinkPyType(SELF_TYPE) \
  DirectReprMethod(Py##SELF_TYPE##_Repr, Py##SELF_TYPE, SELF_TYPE) \
  DirectStrMethod(Py##SELF_TYPE##_Str, Py##SELF_TYPE, SELF_TYPE) \
  DirectCmpBySNLIDMethod(Py##SELF_TYPE##_Cmp, Py##SELF_TYPE) \
  DirectHashMethod(Py##SELF_TYPE##_Hash, Py##SELF_TYPE) \
  extern void Py##SELF_TYPE##_LinkPyType() { \
    PyType##SELF_TYPE.tp_dealloc = (destructor) Py##SELF_TYPE##_DeAlloc; \
    PyType##SELF_TYPE.tp_richcompare = (richcmpfunc)Py##SELF_TYPE##_Cmp; \
    PyType##SELF_TYPE.tp_repr = (reprfunc)Py##SELF_TYPE##_Repr; \
    PyType##SELF_TYPE.tp_str = (reprfunc)Py##SELF_TYPE##_Str; \
    PyType##SELF_TYPE.tp_hash = (hashfunc)Py##SELF_TYPE##_Hash; \
    PyType##SELF_TYPE.tp_methods = Py##SELF_TYPE##_Methods; \
  }

#define PyTypeSNLObjectWithoutSNLIDLinkPyType(SELF_TYPE) \
  DirectReprMethod(Py##SELF_TYPE##_Repr, Py##SELF_TYPE, SELF_TYPE) \
  DirectStrMethod (Py##SELF_TYPE##_Str, Py##SELF_TYPE, SELF_TYPE) \
  DirectCmpByPtrMethod (Py##SELF_TYPE##_Cmp,  Py##SELF_TYPE) \
  DirectHashMethod(Py##SELF_TYPE##_Hash, Py##SELF_TYPE) \
  extern void  Py##SELF_TYPE##_LinkPyType() { \
    PyType##SELF_TYPE.tp_dealloc = (destructor) Py##SELF_TYPE##_DeAlloc; \
    PyType##SELF_TYPE.tp_richcompare = (richcmpfunc) Py##SELF_TYPE##_Cmp; \
    PyType##SELF_TYPE.tp_repr = (reprfunc)Py##SELF_TYPE##_Repr; \
    PyType##SELF_TYPE.tp_str = (reprfunc)Py##SELF_TYPE##_Str; \
    PyType##SELF_TYPE.tp_hash = (hashfunc)Py##SELF_TYPE##_Hash; \
    PyType##SELF_TYPE.tp_methods = Py##SELF_TYPE##_Methods; \
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

#define GetObjectMethod(SELF_TYPE, OBJECT_TYPE, METHOD) \
  static PyObject* PySNL##SELF_TYPE##_##METHOD(PySNL##SELF_TYPE* self) { \
    METHOD_HEAD("SNL##SELF_TYPE.##METHOD##()") \
    return PySNL##OBJECT_TYPE##_Link(selfObject->METHOD()); \
  }

#define GetObjectByName(SELF_TYPE, OBJECT_TYPE) \
  static PyObject* PySNL##SELF_TYPE##_get##OBJECT_TYPE(PySNL##SELF_TYPE* self, PyObject* args) { \
    SNL##OBJECT_TYPE* obj = nullptr; \
    METHOD_HEAD("SNL##SELF_TYPE.get##OBJECT_TYPE()") \
    char* name = NULL; \
    if (PyArg_ParseTuple(args, "s:SNL##SELF_TYPE.get##OBJECT_TYPE", &name)) { \
      SNLTRY \
      obj = selfObject->get##OBJECT_TYPE(SNLName(name)); \
      SNLCATCH \
    } else { \
      setError("invalid number of parameters for get##OBJECT_TYPE."); \
      return nullptr; \
    } \
    return PySNL##OBJECT_TYPE##_Link(obj); \
  }

#define GetObjectByIndex(SELF_TYPE, OBJECT_TYPE, METHOD) \
  static PyObject* PySNL##SELF_TYPE##_get##METHOD(PySNL##SELF_TYPE* self, PyObject* args) { \
    SNL##OBJECT_TYPE* obj = nullptr; \
    METHOD_HEAD("SNL##SELF_TYPE.get##OBJECT_TYPE()") \
    int index = 0; \
    if (PyArg_ParseTuple(args, "i:SNL##SELF_TYPE.get##METHOD", &index)) { \
      SNLTRY \
      obj = selfObject->get##METHOD(index); \
      SNLCATCH \
    } else { \
      setError("invalid number of parameters for get##METHOD."); \
      return nullptr; \
    } \
    return PySNL##OBJECT_TYPE##_Link(obj); \
  }

#define GetNameMethod(SELF_TYPE) \
  static PyObject* Py##SELF_TYPE##_getName(Py##SELF_TYPE* self) { \
    METHOD_HEAD(#SELF_TYPE ".getName()") \
    SNLTRY \
    return PyUnicode_FromString(selfObject->getName().getString().c_str()); \
    SNLCATCH \
    return nullptr; \
  }

#define SetNameMethod(SELF_TYPE) \
  static PyObject* PySNL##SELF_TYPE##_setName(PySNL##SELF_TYPE* self, PyObject* arg) { \
    METHOD_HEAD("SNL" #SELF_TYPE ".setName()") \
    if (not PyUnicode_Check(arg)) { \
      setError("SNL" #SELF_TYPE ".setName() expects a string as argument"); \
      return nullptr; \
    } \
    selfObject->setName(SNLName(PyUnicode_AsUTF8(arg))); \
    Py_RETURN_NONE; \
  }

#define GetStringAttribute(SELF_TYPE, METHOD) \
  static PyObject* PySNL##SELF_TYPE##_##METHOD(PySNL##SELF_TYPE* self) { \
    METHOD_HEAD("SNL##SELF_TYPE.##METHOD##()") \
    SNLTRY \
    return PyUnicode_FromString(selfObject->METHOD().c_str()); \
    SNLCATCH \
    return nullptr; \
  }

#define GetBoolAttribute(SELF_TYPE, METHOD) \
  static PyObject* PySNL##SELF_TYPE##_##METHOD(PySNL##SELF_TYPE* self) { \
    METHOD_HEAD("SNL##SELF_TYPE.##METHOD##()") \
    if (selfObject->METHOD()) Py_RETURN_TRUE; \
    Py_RETURN_FALSE; \
  }

#define GetBoolAttributeWithFunction(SELF_TYPE, METHOD, FUNCTION) \
  static PyObject* PySNL##SELF_TYPE##_##METHOD(PySNL##SELF_TYPE* self) { \
    METHOD_HEAD("SNL##SELF_TYPE.##METHOD##()") \
    if (FUNCTION(selfObject)) Py_RETURN_TRUE; \
    Py_RETURN_FALSE; \
  }

#define LoadObjectConstant(DICTIONARY, CONSTANT_VALUE, CONSTANT_NAME)  \
 constant = PyLong_FromLong((long)CONSTANT_VALUE);                  \
 PyDict_SetItemString(DICTIONARY, CONSTANT_NAME, constant);         \
 Py_DECREF(constant);

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

#define PyTypeObjectDefinitions(SELF_TYPE) \
  PyTypeObject PyType##SELF_TYPE = { \
    PyObject_HEAD_INIT(NULL) \
    #SELF_TYPE, /* tp_name */ \
    sizeof(Py##SELF_TYPE), /* tp_basicsize */                       \
    0, /* tp_itemsize */                                            \
    0, /* tp_dealloc */                                             \
    0, /* tp_print */                                               \
    0, /* tp_getattr */                                             \
    0, /* tp_setattr */                                             \
    0, /* tp_reserved */                                            \
    0, /* tp_repr */                                                \
    0, /* tp_as_number */                                           \
    0, /* tp_as_sequence */                                         \
    0, /* tp_as_mapping */                                          \
    0, /* tp_hash */                                                \
    0, /* tp_call */                                                \
    0, /* tp_str */                                                 \
    0, /* tp_getattro */                                            \
    0, /* tp_setattro */                                            \
    0, /* tp_as_buffer */                                           \
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */        \
    "#SELF_TYPE objects", /* tp_doc */                              \
    0, /* tp_traverse */                                            \
    0, /* tp_clear */                                               \
    0, /* tp_richcompare */                                         \
    0, /* tp_weaklistoffset */                                      \
    0, /* tp_iter */                                                \
    0, /* tp_iternext */                                            \
    0, /* tp_methods */                                             \
    0, /* tp_members */                                             \
    0, /* tp_getset */                                              \
    0, /* tp_base */                                                \
    0, /* tp_dict */                                                \
    0, /* tp_descr_get */                                           \
    0, /* tp_descr_set */                                           \
    0, /* tp_dictoffset */                                          \
    0, /* tp_init */                                                \
    0, /* tp_alloc */                                               \
    PyType_GenericNew /* tp_new */                                  \
};

#define PyTypeInheritedObjectDefinitions(SELF_TYPE, SUPER_TYPE) \
  PyTypeObject PyType##SELF_TYPE = { \
    PyObject_HEAD_INIT(NULL) \
    #SELF_TYPE, /* tp_name */                                       \
    sizeof(Py##SELF_TYPE), /* tp_basicsize */                       \
    0, /* tp_itemsize */                                            \
    0, /* tp_dealloc */                                             \
    0, /* tp_print */                                               \
    0, /* tp_getattr */                                             \
    0, /* tp_setattr */                                             \
    0, /* tp_reserved */                                            \
    0, /* tp_repr */                                                \
    0, /* tp_as_number */                                           \
    0, /* tp_as_sequence */                                         \
    0, /* tp_as_mapping */                                          \
    0, /* tp_hash */                                                \
    0, /* tp_call */                                                \
    0, /* tp_str */                                                 \
    0, /* tp_getattro */                                            \
    0, /* tp_setattro */                                            \
    0, /* tp_as_buffer */                                           \
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */        \
    "#SELF_TYPE objects", /* tp_doc */                              \
    0, /* tp_traverse */                                            \
    0, /* tp_clear */                                               \
    0, /* tp_richcompare */                                         \
    0, /* tp_weaklistoffset */                                      \
    0, /* tp_iter */                                                \
    0, /* tp_iternext */                                            \
    0, /* tp_methods */                                             \
    0, /* tp_members */                                             \
    0, /* tp_getset */                                              \
    &PyType##SUPER_TYPE, /* tp_base */                              \
    0, /* tp_dict */                                                \
    0, /* tp_descr_get */                                           \
    0, /* tp_descr_set */                                           \
    0, /* tp_dictoffset */                                          \
    0, /* tp_init */                                                \
    0, /* tp_alloc */                                               \
    PyType_GenericNew /* tp_new */                                  \
};

#define PyContainerMethods(TYPE, CONTAINER) \
  DirectDeallocMethod(CONTAINER) \
  static PyObject* Py##CONTAINER##IteratorNext(Py##CONTAINER##Iterator* pyIterator) { \
    auto iterator = pyIterator->object_; \
    if (iterator and pyIterator->container_ \
      and pyIterator->container_->object_ \
      and *iterator != pyIterator->container_->object_->end()) { \
      auto object = **iterator; \
      ++(*iterator); \
      return Py##TYPE##_Link(object); \
    } \
    return nullptr; \
  } \
  static PyObject* getIterator(Py##CONTAINER* pyContainer) { \
    auto pyIterator = \
      PyObject_New(Py##CONTAINER##Iterator, &PyType##CONTAINER##Iterator); \
    if (not pyIterator) return nullptr; \
    pyIterator->container_ = pyContainer; \
    pyIterator->object_ = new naja::NajaCollection<TYPE*>::Iterator(pyContainer->object_->begin()); \
    Py_INCREF(pyContainer); \
    return (PyObject*)pyIterator; \
  } \
  static void Py##CONTAINER##IteratorDeAlloc(Py##CONTAINER##Iterator* pyIterator) { \
    Py_XDECREF(pyIterator->container_); \
    if (pyIterator->object_) delete pyIterator->object_; \
    PyObject_Del(pyIterator); \
  } \
  extern void Py##CONTAINER##_LinkPyType() { \
    PyType##CONTAINER.tp_iter               = (getiterfunc)getIterator; \
    PyType##CONTAINER.tp_dealloc            = (destructor)Py##CONTAINER##_DeAlloc; \
    PyType##CONTAINER##Iterator.tp_dealloc  = (destructor)Py##CONTAINER##IteratorDeAlloc; \
    PyType##CONTAINER##Iterator.tp_iter     = PyObject_SelfIter; \
    PyType##CONTAINER##Iterator.tp_iternext = (iternextfunc)Py##CONTAINER##IteratorNext; \
  }

#define PyTypeContainerObjectDefinitions(SELF_TYPE) \
  PyTypeObject PyType##SELF_TYPE = { \
      PyVarObject_HEAD_INIT(NULL,0) \
      #SELF_TYPE                      /* tp_name.          */           \
    , sizeof(Py##SELF_TYPE)           /* tp_basicsize.     */           \
    , 0                               /* tp_itemsize.      */           \
    /* methods. */                                                      \
    , 0                               /* tp_dealloc.       */           \
    , 0                               /* tp_print.         */           \
    , 0                               /* tp_getattr.       */           \
    , 0                               /* tp_setattr.       */           \
    , 0                               /* tp_compare.       */           \
    , 0                               /* tp_repr.          */           \
    , 0                               /* tp_as_number.     */           \
    , 0                               /* tp_as_sequence.   */           \
    , 0                               /* tp_as_mapping.    */           \
    , 0                               /* tp_hash.          */           \
    , 0                               /* tp_call.          */           \
    , 0                               /* tp_str            */           \
    , 0                               /* tp_getattro.      */           \
    , 0                               /* tp_setattro.      */           \
    , 0                               /* tp_as_buffer.     */           \
    , Py_TPFLAGS_DEFAULT              /* tp_flags.         */           \
    , "#SELF_TYPE objects"            /* tp_doc.           */           \
  };

#define GENERIC_METHOD_HEAD(SELF_TYPE, function) \
  if (not self->ACCESS_OBJECT) {                                                            \
    setError("Attempt to call " function " on an unbound object");                          \
    return nullptr;                                                                         \
  }                                                                                         \
  SELF_TYPE* selfObject = dynamic_cast<SELF_TYPE*>(self->ACCESS_OBJECT); \
  if (not selfObject) { \
    setError("Invalid dynamic_cast<> while calling " function "");                          \
    return nullptr;                                                                         \
  }

#define GetContainerMethod(TYPE, ITERATED, CONTAINER, GET_OBJECTS) \
  static PyObject* PySNL##TYPE##_get##GET_OBJECTS(PySNL##TYPE *self) { \
    METHOD_HEAD("SNL" #TYPE ".get" #GET_OBJECTS "()") \
    PySNL##CONTAINER* pyObjects = nullptr; \
    SNLTRY \
    auto objects = new naja::NajaCollection<SNL##ITERATED*>(selfObject->get##GET_OBJECTS()); \
    pyObjects = PyObject_NEW(PySNL##CONTAINER, &PyTypeSNL##CONTAINER); \
    if (not pyObjects) return nullptr; \
    pyObjects->object_ = objects; \
    SNLCATCH \
    return (PyObject*)pyObjects; \
  }

#define GetContainerMethodWithMethodName(TYPE, ITERATED, METHOD) \
  static PyObject* PySNL##TYPE##_##METHOD(PySNL##TYPE *self) { \
    METHOD_HEAD("SNL" #TYPE "." #METHOD "()") \
    PySNL##ITERATED##s* pyObjects = nullptr; \
    SNLTRY \
    auto objects = new naja::NajaCollection<SNL##ITERATED*>(selfObject->METHOD()); \
    pyObjects = PyObject_NEW(PySNL##ITERATED##s, &PyTypeSNL##ITERATED##s); \
    if (not pyObjects) return nullptr; \
    pyObjects->object_ = objects; \
    SNLCATCH \
    return (PyObject*)pyObjects; \
  }

#define GetDesignModelingRelatedObjects(TYPE, GETTER, OWNER_TYPE) \
  if (IsPy##TYPE(object)) { \
    auto object_o = PY##TYPE##_O(object); \
    SNLTRY \
    auto objects = new naja::NajaCollection<TYPE*>(SNLDesignModeling::GETTER(object_o)); \
    auto pyObjects = PyObject_NEW(Py##TYPE##s, &PyType##TYPE##s); \
    if (not pyObjects) return nullptr; \
    pyObjects->object_ = objects; \
    return (PyObject*)pyObjects; \
    SNLCATCH \
  } \
  setError("malformed " #OWNER_TYPE "." #GETTER " method"); \
  return nullptr;

#endif /* __PY_INTERFACE_H */