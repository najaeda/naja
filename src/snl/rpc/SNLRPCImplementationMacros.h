// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_RPC_IMPLEMENTATION_MACROS_H_
#define __SNL_RPC_IMPLEMENTATION_MACROS_H_

namespace {

SNLID::Type najaSNLIDTypeToSNLIDType(naja::SNL::SNLID::Type snlIDType) {
  switch(snlIDType) {
    case naja::SNL::SNLID::Type::DB:
      return SNLID::Type::DB;
    case naja::SNL::SNLID::Type::Library:
      return SNLID::Type::LIBRARY;
    case naja::SNL::SNLID::Type::Design:
      return SNLID::Type::DESIGN;
    case naja::SNL::SNLID::Type::Term:
      return SNLID::Type::TERM;
    case naja::SNL::SNLID::Type::Instance:
      return SNLID::Type::INSTANCE;
    case naja::SNL::SNLID::Type::TermBit:
      return SNLID::Type::TERMBIT;
    case naja::SNL::SNLID::Type::Net:
      return SNLID::Type::NET;
    case naja::SNL::SNLID::Type::NetBit:
      return SNLID::Type::NETBIT;
    case naja::SNL::SNLID::Type::InstTerm:
      return SNLID::Type::INSTTERM;
  }
}

}

#define STRING_METHODS(TYPE)                                                        \
  kj::Promise<void> TYPE::getTypeName(GetTypeNameContext context) {                 \
    if (getObject()) {                                                              \
      context.getResults().setType(getObject()->getTypeName());                     \
    }                                                                               \
    return kj::READY_NOW;                                                           \
  }                                                                                 \
  kj::Promise<void> TYPE::getString(GetStringContext context) {                     \
    if (getObject()) {                                                              \
      context.getResults().setString(getObject()->getString());                     \
    }                                                                               \
    return kj::READY_NOW;                                                           \
  }                                                                                 \
  kj::Promise<void> TYPE::getDescription(GetDescriptionContext context) {           \
    if (getObject()) {                                                              \
      context.getResults().setDescription(getObject()->getDescription());           \
    }                                                                               \
    return kj::READY_NOW;                                                           \
  }

#define SNLID_METHOD(TYPE) \
  kj::Promise<void> TYPE::getSNLID(GetSNLIDContext context) { \
    ::capnp::MallocMessageBuilder message; \
    SNLID::Builder id = message.initRoot<SNLID>();              \
    auto snlID = getObject()->getSNLID();                       \
    id.setType(najaSNLIDTypeToSNLIDType(snlID.type_)); \
    id.setDbID(snlID.dbID_);                                    \
    id.setLibraryID(snlID.libraryID_);                          \
    id.setDesignID(snlID.designID_);                            \
    id.setDesignObjectID(snlID.designObjectID_);                \
    id.setInstanceID(snlID.instanceID_); \
    id.setBit(snlID.bit_); \
    context.getResults().setSnlid(id);                          \
    return kj::READY_NOW;                                       \
  }

#define ID_METHOD(TYPE) \
  kj::Promise<void> TYPE::getID(GetIDContext context) {         \
    if (object_) {                                              \
      context.getResults().setId(object_->getID());             \
    }                                                           \
    return kj::READY_NOW;                                       \
  }

#define NAME_METHOD(TYPE, GET_OBJECT) \
  kj::Promise<void> TYPE::getName(GetNameContext context) { \
    if (GET_OBJECT) { \
      context.getResults().setIsAnonymous(GET_OBJECT->isAnonymous()); \
      if (not GET_OBJECT->isAnonymous()) {                            \
        context.getResults().setName(GET_OBJECT->getName().getString()); \
      }                                                               \
    }                                                                 \
    return kj::READY_NOW;                                             \
  }

#define SIMPLE_GET_METHOD(TYPE, GET_OBJECT, GET_TYPE, SET_CONTEXT_OBJECT)       \
  kj::Promise<void> TYPE::get##GET_TYPE(Get##GET_TYPE##Context context) {       \
    if (GET_OBJECT) {                                                           \
      context.getResults().set##SET_CONTEXT_OBJECT(GET_OBJECT->get##GET_TYPE());\
    }                                                                           \
    return kj::READY_NOW;                                                       \
  }

#define GET_OBJECTS_METHOD(OBJECT_TYPE, GET_OBJECT, TYPE, TYPES)                        \
  kj::Promise<void> OBJECT_TYPE::get##TYPES(Get##TYPES##Context context) {              \
    if (GET_OBJECT) {                                                                   \
      auto objects = context.getResults().init##TYPES(GET_OBJECT->get##TYPES().size()); \
      unsigned index = 0;                                                               \
      for (auto snl##TYPE: GET_OBJECT->get##TYPES()) {                                  \
        objects.set(index++, kj::heap<SNL##TYPE##Impl>(snl##TYPE));                     \
      }                                                                                 \
    }                                                                                   \
    return kj::READY_NOW;                                                               \
  }

#define GET_BUS_OBJECTS_METHOD(OBJECT_TYPE, TYPE, BUSTYPE, SCALARTYPE)                  \
kj::Promise<void> OBJECT_TYPE::get##TYPE##s(Get##TYPE##sContext context) {              \
  if (object_) {                                                                        \
    auto objects = context.getResults().init##TYPE##s(object_->get##TYPE##s().size());  \
    unsigned index = 0;                                                                 \
    for (auto snl##TYPE: object_->get##TYPE##s()) {                                     \
      if (auto scalar##TYPE = dynamic_cast<naja::SNL::SNLScalar##TYPE*>(snl##TYPE)) { \
        objects[index].setIsBus(false);                                                 \
        objects[index].set##TYPE(kj::heap<SNL##SCALARTYPE##Impl>(scalar##TYPE));        \
      } else {                                                                          \
        auto bus##TYPE = static_cast<naja::SNL::SNLBus##TYPE*>(snl##TYPE);              \
        objects[index].setIsBus(true);                                                  \
        objects[index].set##TYPE(kj::heap<SNL##BUSTYPE##Impl>(bus##TYPE));              \
      }                                                                                 \
      ++index;                                                                          \
    }                                                                                   \
  }                                                                                     \
  return kj::READY_NOW;                                                                 \
}

#define SNLOBJECT_METHODS(TYPE) \
  STRING_METHODS(TYPE)

#define SNLIDOBJECT_METHODS(TYPE) \
  SNLOBJECT_METHODS(TYPE)         \
  SNLID_METHOD(TYPE)

#define SNLDESIGNOBJECT_METHODS(TYPE)                                                     \
  SNLIDOBJECT_METHODS(TYPE)                                                               \
  kj::Promise<void> TYPE::getDesign(GetDesignContext context) {                           \
    context.getResults().setDesign(kj::heap<SNLDesignImpl>(getObject()->getDesign()));    \
    return kj::READY_NOW;                                                                 \
  }
  
#define REFERENCE_METHOD(TYPE, GET_OBJECT) \
  kj::Promise<void> TYPE::getReference(GetReferenceContext context) {                     \
    ::capnp::MallocMessageBuilder message;                                                \
    SNLDesignObjectReference::Builder ref = message.initRoot<SNLDesignObjectReference>(); \
    auto snlRef = GET_OBJECT->getReference();                                             \
    ref.setDbID(snlRef.dbID_);                                                            \
    ref.setLibraryID(snlRef.libraryID_);                                                  \
    ref.setDesignID(snlRef.designID_);                                                    \
    ref.setDesignObjectID(snlRef.designObjectID_);                                        \
    context.getResults().setReference(ref);                                               \
    return kj::READY_NOW;                                                                 \
  }

#endif /* __SNL_RPC_IMPLEMENTATION_MACROS_H_ */
