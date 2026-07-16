// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLInstance.h"

#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLNet.h"
#include "NLDB0.h"

#include "PyInterface.h"

#include "PySNLDesign.h"
#include "PySNLInstParameter.h"
#include "PySNLInstTerm.h"
#include "PySNLBitTerm.h"
#include "PySNLInstTerms.h"
#include "PySNLInstParameters.h"
#include "PySNLNet.h"

#include "SNLDesignModeling.h"

namespace PYNAJA {

using namespace naja::NL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT           parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLInstance, function)

static PyObject* PySNLInstance_create(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  PyObject* arg1 = nullptr;
  const char* arg2 = nullptr;
  if (not PyArg_ParseTuple(args, "OO|s:SNLDB.create", &arg0, &arg1, &arg2)) {
    setError("malformed SNLInstance create method");
    return nullptr;
  }
  NLName name;
  if (arg2) {
    name = NLName(arg2);
  }

  SNLInstance* instance = nullptr;
  TRY
  if (not IsPySNLDesign(arg0)) {
    setError("SNLInstance create needs SNLDesign as first argument");
    return nullptr;
  }
  if (not IsPySNLDesign(arg1)) {
    setError("SNLInstance create needs SNLDesign as second argument");
    return nullptr;
  }
  instance = SNLInstance::create(PYSNLDesign_O(arg0), PYSNLDesign_O(arg1), name);
  NLCATCH
  return PySNLInstance_Link(instance);
}

static PyObject* PySNLInstance_createConstantDriver(PyObject*, PyObject* args) {
  PyObject* netArg = nullptr;
  const char* valueArg = nullptr;
  const char* kindArg = "assign";
  const char* nameArg = nullptr;
  if (not PyArg_ParseTuple(
        args, "Os|ss:SNLInstance.createConstantDriver",
        &netArg, &valueArg, &kindArg, &nameArg)) {
    setError("malformed SNLInstance.createConstantDriver method");
    return nullptr;
  }
  if (not IsPySNLNet(netArg)) {
    setError("SNLInstance.createConstantDriver needs an SNLNet as first argument");
    return nullptr;
  }
  NLConstantDriverKind kind;
  const std::string kindString(kindArg);
  if (kindString == "assign") {
    kind = NLConstantDriverKind::Assign;
  } else if (kindString == "supply") {
    kind = NLConstantDriverKind::Supply;
  } else {
    setError("constant driver kind must be 'assign' or 'supply'");
    return nullptr;
  }
  SNLInstance* instance = nullptr;
  TRY
  auto* net = PYSNLNet_O(netArg);
  auto value = NLLogicVector::fromVerilogBinary(valueArg);
  if (value.getWidth() != net->getWidth()) {
    throw NLException("constant driver value width does not match net width");
  }
  instance = SNLDesignModeling::createConstantDriver(
    net->getDesign(), value, kind, nameArg ? NLName(nameArg) : NLName());
  instance->setTermNet(NLDB0::getConstOutput(instance->getModel()), net);
  NLCATCH
  return PySNLInstance_Link(instance);
}

GetObjectMethod(SNLInstance, SNLDesign, getModel)
GetObjectByName(SNLInstance, SNLInstParameter, getInstParameter)

static PyObject* PySNLInstance_getCombinatorialInputs(PySNLDesign*, PyObject* output) {
  if (IsPySNLInstTerm(output)) {
    auto outputITerm = PYSNLInstTerm_O(output);
    PySNLInstTerms* pyObjects = nullptr;
    TRY
    auto objects = new naja::NajaCollection<SNLInstTerm*>(SNLDesignModeling::getCombinatorialInputs(outputITerm));
    pyObjects = PyObject_NEW(PySNLInstTerms, &PyTypeSNLInstTerms);
    if (not pyObjects) return nullptr;
    pyObjects->object_ = objects;
    NLCATCH
    return (PyObject*)pyObjects;
  }
  setError("malformed SNLInstance.getCombinatorialInputs method");
  return nullptr;
}

static PyObject* PySNLInstance_getCombinatorialOutputs(PySNLDesign*, PyObject* input) {
  if (IsPySNLInstTerm(input)) {
    auto inputITerm = PYSNLInstTerm_O(input);
    PySNLInstTerms* pyObjects = nullptr;
    TRY
    auto objects = new naja::NajaCollection<SNLInstTerm*>(SNLDesignModeling::getCombinatorialOutputs(inputITerm));
    pyObjects = PyObject_NEW(PySNLInstTerms, &PyTypeSNLInstTerms);
    if (not pyObjects) return nullptr;
    pyObjects->object_ = objects;
    NLCATCH
    return (PyObject*)pyObjects;
  }
  setError("malformed SNLInstance.getCombinatorialOutputs method");
  return nullptr;
}

GetNameMethod(SNLInstance)

DBoLinkCreateMethod(SNLInstance)
DBoDeallocMethod(SNLInstance)

PyTypeInheritedObjectDefinitions(SNLInstance, SNLDesignObject)

static PyObject* PySNLInstance_getInstTerm(PySNLInstance* self, PyObject* args) {
  SNLInstTerm* obj = nullptr;
  METHOD_HEAD("SNLInstance.getInstTerm()")
  PySNLBitTerm* pyBitTerm = nullptr;
  if (PyArg_ParseTuple(args, "O!:SNLInstance.getInstTerm", &PyTypeSNLBitTerm, &pyBitTerm)) {
    TRY
    auto bitTerm = PYSNLBitTerm_O(pyBitTerm);
    if (bitTerm) {
      obj = selfObject->getInstTerm(bitTerm);
    }
    NLCATCH
  } else {
    setError("invalid number of parameters for getInstTerm.");
    return nullptr;
  }
  return PySNLInstTerm_Link(obj);
}

GetContainerMethod(SNLInstance, SNLInstTerm*, SNLInstTerms, InstTerms)
GetContainerMethod(SNLInstance, SNLInstParameter*, SNLInstParameters, InstParameters)

DirectGetNumericMethod(PySNLInstance_getID, getID, PySNLInstance, SNLInstance)
GetBoolAttribute(SNLInstance, isAssign)
GetBoolAttribute(SNLInstance, isConstantDriver)
GetBoolAttribute(SNLInstance, isRegular)

static PyObject* PySNLInstance_hasInit(PySNLInstance* self) {
  METHOD_HEAD("SNLInstance.hasInit()")
  bool result = false;
  TRY
  result = SNLDesignModeling::hasInit(selfObject);
  NLCATCH
  if (result) Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}

static PyObject* PySNLInstance_getInitValue(PySNLInstance* self) {
  METHOD_HEAD("SNLInstance.getInitValue()")
  std::optional<NLLogicVector> value;
  TRY
  value = SNLDesignModeling::getInitValue(selfObject);
  NLCATCH
  if (!value) Py_RETURN_NONE;
  return PyUnicode_FromString(value->toVerilogBinary().c_str());
}

static PyObject* PySNLInstance_getResetValue(PySNLInstance* self) {
  METHOD_HEAD("SNLInstance.getResetValue()")
  std::optional<NLLogicVector> value;
  TRY
  value = SNLDesignModeling::getResetValue(selfObject);
  NLCATCH
  if (!value) Py_RETURN_NONE;
  return PyUnicode_FromString(value->toVerilogBinary().c_str());
}

PyMethodDef PySNLInstance_Methods[] = {
  { "create", (PyCFunction)PySNLInstance_create, METH_VARARGS|METH_STATIC,
    "SNLInstance creator"},
  { "createConstantDriver", (PyCFunction)PySNLInstance_createConstantDriver,
    METH_VARARGS|METH_STATIC,
    "create and connect a typed constant driver to an SNLNet"},
  { "getName", (PyCFunction)PySNLInstance_getName, METH_NOARGS,
    "get SNLInstance name"},
  { "getID", (PyCFunction)PySNLInstance_getID, METH_NOARGS,
    "get the ID."},
  {"getModel", (PyCFunction)PySNLInstance_getModel, METH_NOARGS,
    "Returns the SNLInstance model SNLDesign."},
  {"getInstParameter", (PyCFunction)PySNLInstance_getInstParameter, METH_VARARGS,
    "Returns the SNLInstParameter by name."},
  {"getInstTerm", (PyCFunction)PySNLInstance_getInstTerm, METH_VARARGS,
    "Returns the SNLInstTerm corresponding to a model's SNLBitTerm."},
  {"getInstTerms", (PyCFunction)PySNLInstance_getInstTerms, METH_NOARGS,
    "get a container of SNLInstTerms."},
  {"getInstParameters", (PyCFunction)PySNLInstance_getInstParameters, METH_NOARGS,
    "get a container of SNLInstParameters."},
  {"hasInit", (PyCFunction)PySNLInstance_hasInit, METH_NOARGS,
    "whether this state element has explicit power-up initialization."},
  {"getInitValue", (PyCFunction)PySNLInstance_getInitValue, METH_NOARGS,
    "get explicit power-up initialization as a canonical binary literal, or None."},
  {"getResetValue", (PyCFunction)PySNLInstance_getResetValue, METH_NOARGS,
    "get reset value as a canonical binary literal, or None."},
  { "isAssign", (PyCFunction)PySNLInstance_isAssign, METH_NOARGS,
    "whether this is an assign-glue instance."},
  { "isConstantDriver", (PyCFunction)PySNLInstance_isConstantDriver, METH_NOARGS,
    "whether this is a constant-driver instance."},
  { "isRegular", (PyCFunction)PySNLInstance_isRegular, METH_NOARGS,
    "whether this is neither assign glue nor a constant driver."},
  { "getCombinatorialInputs", (PyCFunction)PySNLInstance_getCombinatorialInputs, METH_O|METH_STATIC,
    "get combinatorial inputs of an instance term"},
  { "getCombinatorialOutputs", (PyCFunction)PySNLInstance_getCombinatorialOutputs, METH_O|METH_STATIC,
    "get combinatorial outputs of an instance term"},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

PyTypeNLFinalObjectWithNLIDLinkPyType(SNLInstance)

}
