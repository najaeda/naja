// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_RPC_DECLARATION_MACROS_H_
#define __SNL_RPC_DECLARATION_MACROS_H_

#define STRING_DECLARATION_METHODS                                              \
  kj::Promise<void> getTypeName(GetTypeNameContext context) override;           \
  kj::Promise<void> getString(GetStringContext context) override;               \
  kj::Promise<void> getDescription(GetDescriptionContext context) override;

#define SNLID_DECLARATION_METHODS                               \
  kj::Promise<void> getSNLID(GetSNLIDContext context) override; \
  
  
  //kj::Promise<void> getID(GetIDContext context) override;

#define NAME_DECLARATION_METHOD \
  kj::Promise<void> getName(GetNameContext context) override;

#define GET_OBJECTS_DECLARATION_METHOD(TYPES) \
  kj::Promise<void> get##TYPES(Get##TYPES##Context context) override;

#define SIMPLE_GET_DECLARATION_METHOD(GET_OBJECT) \
  kj::Promise<void> get##GET_OBJECT(Get##GET_OBJECT##Context context) override;

#define SNLOBJECT_DECLARATION_METHODS   \
  STRING_DECLARATION_METHODS

#define SNLIDOBJECT_DECLARATION_METHODS \
  SNLOBJECT_DECLARATION_METHODS         \
  SNLID_DECLARATION_METHODS

#define SNLDESIGNOBJECT_DECLARATION_METHODS                             \
  SNLIDOBJECT_DECLARATION_METHODS                                       \
  kj::Promise<void> getDesign(GetDesignContext context) override;

#define REFERENCE_DECLARATION_METHOD \
  kj::Promise<void> getReference(GetReferenceContext context) override;

#endif /* __SNL_RPC_DECLARATION_MACROS_H_ */
