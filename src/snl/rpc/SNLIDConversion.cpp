// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLIDConversion.h"

namespace {

naja::SNL::SNLID::Type snlIDTypeToNajaSNLIDType(SNLID::Type type) {
  switch (type) {
    case SNLID::Type::DB:
      return naja::SNL::SNLID::Type::DB;
    case SNLID::Type::LIBRARY:
      return naja::SNL::SNLID::Type::Library;
    case SNLID::Type::DESIGN:
      return naja::SNL::SNLID::Type::Design;
    case SNLID::Type::INSTANCE:
      return naja::SNL::SNLID::Type::Instance;
    case SNLID::Type::TERM:
      return naja::SNL::SNLID::Type::Term;
    case SNLID::Type::TERMBIT:
      return naja::SNL::SNLID::Type::TermBit;
    case SNLID::Type::NET:
      return naja::SNL::SNLID::Type::Net;
    case SNLID::Type::NETBIT:
      return naja::SNL::SNLID::Type::NetBit;
    case SNLID::Type::INSTTERM:
      return naja::SNL::SNLID::Type::InstTerm;
  }
  //avoid compilation warning
  return naja::SNL::SNLID::Type::DB;
}

SNLID::Type najaSNLIDTypeToSNLIDType(naja::SNL::SNLID::Type type) {
  switch (type) {
    case naja::SNL::SNLID::Type::DB:
      return SNLID::Type::DB;
    case naja::SNL::SNLID::Type::Library:
      return SNLID::Type::LIBRARY;
    case naja::SNL::SNLID::Type::Design:
      return SNLID::Type::DESIGN;
    case naja::SNL::SNLID::Type::Instance:
      return SNLID::Type::INSTANCE;
    case naja::SNL::SNLID::Type::Term:
      return SNLID::Type::TERM;
    case naja::SNL::SNLID::Type::TermBit:
      return SNLID::Type::TERMBIT;
    case naja::SNL::SNLID::Type::Net:
      return SNLID::Type::NET;
    case naja::SNL::SNLID::Type::NetBit:
      return SNLID::Type::NETBIT;
    case naja::SNL::SNLID::Type::InstTerm:
      return SNLID::Type::INSTTERM;
  }
  //avoid compilation warning
  return SNLID::Type::DB;
}

}

void SNLIDConversion::najaSNLIDToSNLID(const naja::SNL::SNLID& najaSNLID, SNLID::Builder& snlid) {
  snlid.setType(najaSNLIDTypeToSNLIDType(najaSNLID.type_));
  snlid.setDbID(najaSNLID.dbID_);
  snlid.setLibraryID(najaSNLID.libraryID_);
  snlid.setDesignID(najaSNLID.designID_);
  snlid.setDesignObjectID(najaSNLID.designObjectID_);
  snlid.setInstanceID(najaSNLID.instanceID_);
  snlid.setBit(najaSNLID.bit_);
}

naja::SNL::SNLID SNLIDConversion::snlIDToNajaSNLID(const SNLID::Reader& snlid) {
  return naja::SNL::SNLID(
    snlIDTypeToNajaSNLIDType(snlid.getType()),
    snlid.getDbID(),
    snlid.getLibraryID(),
    snlid.getDesignID(),
    snlid.getDesignObjectID(),
    snlid.getInstanceID(),
    snlid.getBit()
  );
}
