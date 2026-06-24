// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PyNLID.h"

#include <array>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>

#include "PyInterface.h"

namespace PYNAJA {

using namespace naja::NL;

namespace {

constexpr auto kNLIDFields = 7;

bool isValidType(unsigned long long value) {
  return value >= static_cast<unsigned long long>(NLID::Type::DB) and
    value <= static_cast<unsigned long long>(NLID::Type::InstTerm);
}

bool parseUnsignedField(
    PyObject* object,
    const char* fieldName,
    unsigned long long maxValue,
    unsigned long long& value) {
  if (not PyLong_Check(object)) {
    std::ostringstream error;
    error << "NLID " << fieldName << " must be an integer";
    setError(error.str());
    return false;
  }
  value = PyLong_AsUnsignedLongLong(object);
  if (PyErr_Occurred()) {
    return false;
  }
  if (value > maxValue) {
    std::ostringstream error;
    error << "NLID " << fieldName << " is out of range";
    setError(error.str());
    return false;
  }
  return true;
}

bool parseBitField(PyObject* object, NLID::Bit& value) {
  if (not PyLong_Check(object)) {
    setError("NLID bit must be an integer");
    return false;
  }
  long long bit = PyLong_AsLongLong(object);
  if (PyErr_Occurred()) {
    return false;
  }
  if (bit < std::numeric_limits<NLID::Bit>::min() or
      bit > std::numeric_limits<NLID::Bit>::max()) {
    setError("NLID bit is out of range");
    return false;
  }
  value = static_cast<NLID::Bit>(bit);
  return true;
}

NLID* buildNLIDFromObjects(const std::array<PyObject*, kNLIDFields>& fields) {
  unsigned long long type = 0;
  unsigned long long dbID = 0;
  unsigned long long libraryID = 0;
  unsigned long long designID = 0;
  unsigned long long designObjectID = 0;
  unsigned long long instanceID = 0;
  NLID::Bit bit = 0;

  if (not parseUnsignedField(fields[0], "type", std::numeric_limits<uint8_t>::max(), type)) {
    return nullptr;
  }
  if (not isValidType(type)) {
    setError("NLID type is out of range");
    return nullptr;
  }
  if (not parseUnsignedField(fields[1], "dbID", std::numeric_limits<NLID::DBID>::max(), dbID) or
      not parseUnsignedField(fields[2], "libraryID", std::numeric_limits<NLID::LibraryID>::max(), libraryID) or
      not parseUnsignedField(fields[3], "designID", std::numeric_limits<NLID::DesignID>::max(), designID) or
      not parseUnsignedField(fields[4], "designObjectID", std::numeric_limits<NLID::DesignObjectID>::max(), designObjectID) or
      not parseUnsignedField(fields[5], "instanceID", std::numeric_limits<NLID::DesignObjectID>::max(), instanceID) or
      not parseBitField(fields[6], bit)) {
    return nullptr;
  }

  return new NLID(
      static_cast<NLID::Type>(type),
      static_cast<NLID::DBID>(dbID),
      static_cast<NLID::LibraryID>(libraryID),
      static_cast<NLID::DesignID>(designID),
      static_cast<NLID::DesignObjectID>(designObjectID),
      static_cast<NLID::DesignObjectID>(instanceID),
      bit);
}

std::string getCompactString(const NLID& id) {
  std::ostringstream stream;
  stream << "NLID("
    << static_cast<unsigned>(id.type_) << ":"
    << static_cast<unsigned>(id.dbID_) << ":"
    << id.libraryID_ << ":"
    << id.designID_ << ":"
    << id.designObjectID_ << ":"
    << id.instanceID_ << ":"
    << id.bit_ << ")";
  return stream.str();
}

PyObject* makeBool(bool value) {
  if (value) {
    Py_RETURN_TRUE;
  }
  Py_RETURN_FALSE;
}

void addTypeConstant(PyObject* dictionary, PyObject* typeObject, const char* name, NLID::Type type) {
  PyObject* value = PYNAJA::toPyLong(type);
  PyDict_SetItemString(dictionary, name, value);
  if (typeObject) {
    PyObject_SetAttrString(typeObject, name, value);
  }
  Py_DECREF(value);
}

} // namespace

static int PyNLID_Init(PyNLID* self, PyObject* args, PyObject*) {
  std::array<PyObject*, kNLIDFields> fields = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
  if (not PyArg_ParseTuple(
      args,
      "OOOOOOO:NLID",
      &fields[0],
      &fields[1],
      &fields[2],
      &fields[3],
      &fields[4],
      &fields[5],
      &fields[6])) {
    setError("NLID constructor expects seven integer arguments");
    return -1;
  }
  auto nlid = buildNLIDFromObjects(fields);
  if (not nlid) {
    return -1;
  }
  delete self->object_;
  self->object_ = nlid;
  return 0;
}

static void PyNLID_DeAlloc(PyNLID* self) {
  delete self->object_;
  PyObject_DEL(self);
}

PyObject* PyNLID_Link(const NLID& id) {
  PyNLID* pyObject = PyObject_NEW(PyNLID, &PyTypeNLID);
  if (not pyObject) {
    return nullptr;
  }
  pyObject->object_ = new NLID(id);
  return reinterpret_cast<PyObject*>(pyObject);
}

static PyObject* PyNLID_Repr(PyNLID* self) {
  if (not self->object_) {
    return PyUnicode_FromString("<NLID unbound>");
  }
  return PyUnicode_FromString(getCompactString(*self->object_).c_str());
}

static PyObject* PyNLID_Str(PyNLID* self) {
  return PyNLID_Repr(self);
}

static PyObject* PyNLID_RichCompare(PyNLID* self, PyObject* other, int op) {
  if (not IsPyNLID(other)) {
    Py_RETURN_NOTIMPLEMENTED;
  }
  return richCompare(*self->object_, PYNLID_O(other), op);
}

static Py_hash_t PyNLID_Hash(PyNLID* self) {
  const auto& id = *self->object_;
  Py_hash_t hash = 0x345678;
  auto mix = [&hash](auto value) {
    hash = (hash ^ static_cast<Py_hash_t>(value)) * 1000003;
  };
  mix(static_cast<unsigned>(id.type_));
  mix(id.dbID_);
  mix(id.libraryID_);
  mix(id.designID_);
  mix(id.designObjectID_);
  mix(id.instanceID_);
  mix(id.bit_);
  hash ^= kNLIDFields;
  return hash == -1 ? -2 : hash;
}

static PyObject* PyNLID_FromString(PyObject*, PyObject* arg) {
  if (not PyUnicode_Check(arg)) {
    setError("NLID.from_string expects a string");
    return nullptr;
  }
  const std::string value = PyUnicode_AsUTF8(arg);
  constexpr std::string_view prefix = "NLID(";
  if (value.rfind(prefix, 0) != 0 or value.empty() or value.back() != ')') {
    setError("NLID.from_string expects NLID(t:db:lib:design:object:instance:bit)");
    return nullptr;
  }

  std::array<unsigned long long, kNLIDFields - 1> unsignedFields = {};
  long long bit = 0;
  char separators[kNLIDFields - 1] = {};
  std::istringstream stream(value.substr(prefix.size(), value.size() - prefix.size() - 1));
  stream
    >> unsignedFields[0] >> separators[0]
    >> unsignedFields[1] >> separators[1]
    >> unsignedFields[2] >> separators[2]
    >> unsignedFields[3] >> separators[3]
    >> unsignedFields[4] >> separators[4]
    >> unsignedFields[5] >> separators[5]
    >> bit;
  if (stream.fail() or
      separators[0] != ':' or separators[1] != ':' or separators[2] != ':' or
      separators[3] != ':' or separators[4] != ':' or separators[5] != ':' or
      not stream.eof()) {
    setError("NLID.from_string expects NLID(t:db:lib:design:object:instance:bit)");
    return nullptr;
  }
  PyObject* args = Py_BuildValue(
      "(KKKKKKL)",
      unsignedFields[0],
      unsignedFields[1],
      unsignedFields[2],
      unsignedFields[3],
      unsignedFields[4],
      unsignedFields[5],
      bit);
  if (not args) {
    return nullptr;
  }
  PyObject* nlid = PyObject_CallObject(reinterpret_cast<PyObject*>(&PyTypeNLID), args);
  Py_DECREF(args);
  return nlid;
}

static PyObject* PyNLID_ToTuple(PyNLID* self) {
  const auto& id = *self->object_;
  return Py_BuildValue(
      "(KKKKKKl)",
      static_cast<unsigned long long>(id.type_),
      static_cast<unsigned long long>(id.dbID_),
      static_cast<unsigned long long>(id.libraryID_),
      static_cast<unsigned long long>(id.designID_),
      static_cast<unsigned long long>(id.designObjectID_),
      static_cast<unsigned long long>(id.instanceID_),
      static_cast<long>(id.bit_));
}

static PyObject* PyNLID_GetType(PyNLID* self) {
  return PYNAJA::toPyLong(self->object_->type_);
}

static PyObject* PyNLID_GetDBID(PyNLID* self) {
  return PYNAJA::toPyLong(self->object_->dbID_);
}

static PyObject* PyNLID_GetLibraryID(PyNLID* self) {
  return PYNAJA::toPyLong(self->object_->libraryID_);
}

static PyObject* PyNLID_GetDesignID(PyNLID* self) {
  return PYNAJA::toPyLong(self->object_->designID_);
}

static PyObject* PyNLID_GetDesignObjectID(PyNLID* self) {
  return PYNAJA::toPyLong(self->object_->designObjectID_);
}

static PyObject* PyNLID_GetInstanceID(PyNLID* self) {
  return PYNAJA::toPyLong(self->object_->instanceID_);
}

static PyObject* PyNLID_GetBit(PyNLID* self) {
  return PYNAJA::toPyLong(self->object_->bit_);
}

static PyObject* PyNLID_IsInstance(PyNLID* self) {
  return makeBool(self->object_->type_ == NLID::Type::Instance);
}

static PyObject* PyNLID_IsNet(PyNLID* self) {
  return makeBool(self->object_->type_ == NLID::Type::Net or self->object_->type_ == NLID::Type::NetBit);
}

static PyObject* PyNLID_IsTerm(PyNLID* self) {
  return makeBool(
      self->object_->type_ == NLID::Type::Term or
      self->object_->type_ == NLID::Type::TermBit or
      self->object_->type_ == NLID::Type::InstTerm);
}

static PyObject* PyNLID_IsDesign(PyNLID* self) {
  return makeBool(self->object_->type_ == NLID::Type::Design);
}

PyMethodDef PyNLID_Methods[] = {
  { "from_string", (PyCFunction)PyNLID_FromString, METH_O | METH_STATIC,
    "Create an NLID from its string representation."},
  { "toTuple", (PyCFunction)PyNLID_ToTuple, METH_NOARGS,
    "Return the seven NLID fields as a tuple."},
  { "getType", (PyCFunction)PyNLID_GetType, METH_NOARGS,
    "Return the NLID type."},
  { "getDBID", (PyCFunction)PyNLID_GetDBID, METH_NOARGS,
    "Return the DB id."},
  { "getLibraryID", (PyCFunction)PyNLID_GetLibraryID, METH_NOARGS,
    "Return the library id."},
  { "getDesignID", (PyCFunction)PyNLID_GetDesignID, METH_NOARGS,
    "Return the design id."},
  { "getDesignObjectID", (PyCFunction)PyNLID_GetDesignObjectID, METH_NOARGS,
    "Return the design object id."},
  { "getInstanceID", (PyCFunction)PyNLID_GetInstanceID, METH_NOARGS,
    "Return the instance id."},
  { "getBit", (PyCFunction)PyNLID_GetBit, METH_NOARGS,
    "Return the bit id."},
  { "isInstance", (PyCFunction)PyNLID_IsInstance, METH_NOARGS,
    "Return whether this id identifies an instance."},
  { "isNet", (PyCFunction)PyNLID_IsNet, METH_NOARGS,
    "Return whether this id identifies a net or net bit."},
  { "isTerm", (PyCFunction)PyNLID_IsTerm, METH_NOARGS,
    "Return whether this id identifies a term, term bit, or inst term."},
  { "isDesign", (PyCFunction)PyNLID_IsDesign, METH_NOARGS,
    "Return whether this id identifies a design."},
  {NULL, NULL, 0, NULL}
};

void PyNLID_LinkPyType() {
  PyTypeNLID.tp_dealloc = reinterpret_cast<destructor>(PyNLID_DeAlloc);
  PyTypeNLID.tp_init = reinterpret_cast<initproc>(PyNLID_Init);
  PyTypeNLID.tp_richcompare = reinterpret_cast<richcmpfunc>(PyNLID_RichCompare);
  PyTypeNLID.tp_repr = reinterpret_cast<reprfunc>(PyNLID_Repr);
  PyTypeNLID.tp_str = reinterpret_cast<reprfunc>(PyNLID_Str);
  PyTypeNLID.tp_hash = reinterpret_cast<hashfunc>(PyNLID_Hash);
  PyTypeNLID.tp_methods = PyNLID_Methods;
}

void PyNLID_postModuleInit() {
  PyObject* dictionary = PyTypeNLID.tp_dict;
  PyObject* typeObject = PyModule_New("NLID.Type");
  PyDict_SetItemString(dictionary, "Type", typeObject);

  addTypeConstant(dictionary, typeObject, "DB", NLID::Type::DB);
  addTypeConstant(dictionary, typeObject, "Library", NLID::Type::Library);
  addTypeConstant(dictionary, typeObject, "Design", NLID::Type::Design);
  addTypeConstant(dictionary, typeObject, "Term", NLID::Type::Term);
  addTypeConstant(dictionary, typeObject, "TermBit", NLID::Type::TermBit);
  addTypeConstant(dictionary, typeObject, "Net", NLID::Type::Net);
  addTypeConstant(dictionary, typeObject, "NetBit", NLID::Type::NetBit);
  addTypeConstant(dictionary, typeObject, "Instance", NLID::Type::Instance);
  addTypeConstant(dictionary, typeObject, "InstTerm", NLID::Type::InstTerm);

  Py_DECREF(typeObject);
}

PyTypeObject PyTypeNLID = {
  .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "NLID",
  .tp_basicsize = sizeof(PyNLID),
  .tp_itemsize = 0,
  .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
  .tp_doc = PyDoc_STR("NLID objects"),
  .tp_new = PyType_GenericNew,
};

} // namespace PYNAJA
