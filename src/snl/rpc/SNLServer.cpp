/*
 * Copyright 2022 The Naja Authors.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "SNLServer.h"

#include <iostream>
#include <capnp/ez-rpc.h>

#include "snl_rpc.capnp.h"

#include "SNLUniverse.h"
#include "SNLBusTerm.h"
#include "SNLScalarTerm.h"
#include "SNLBusTermBit.h"
#include "SNLInstTerm.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"

#include "SNLIDConversion.h"
#include "SNLDBImpl.h"
#include "SNLLibraryImpl.h"
#include "SNLDesignImpl.h"
#include "SNLScalarTermImpl.h"
#include "SNLBusTermImpl.h"
#include "SNLBusTermBitImpl.h"
#include "SNLInstTermImpl.h"
#include "SNLNetImpl.h"
#include "SNLBusNetBitImpl.h"
#include "SNLBusNetImpl.h"
#include "SNLInstanceImpl.h"

namespace {

#define GET_OBJECT(TYPE)                                                                        \
  kj::Promise<void> get##TYPE(Get##TYPE##Context context) override {                            \
    auto universe = naja::SNL::SNLUniverse::get();                                              \
    if (universe) {                                                                             \
      auto ref = context.getParams().getReference();                                            \
      naja::SNL::SNLID::DesignObjectReference                                                   \
        snlRef(ref.getDbID(), ref.getLibraryID(), ref.getDesignID(), ref.getDesignObjectID());  \
      auto object = universe->get##TYPE(snlRef);                                                \
      if (object) {                                                                             \
        context.getResults().set##TYPE(kj::heap<SNL##TYPE##Impl>(object));                      \
      } else { \
        context.getResults().setError( \
          "No " + std::string(#TYPE) + " can be found for reference: " + snlRef.getString()     \
        ); \
      }                                                                                         \
    }                                                                                           \
    return kj::READY_NOW;                                                                       \
  }

#define GET_BUS_OBJECT(TYPE, SCALAR_TYPE, BUS_TYPE) \
  kj::Promise<void> get##TYPE(Get##TYPE##Context context) override {                            \
    auto universe = naja::SNL::SNLUniverse::get();                                              \
    if (universe) {                                                                             \
      auto ref = context.getParams().getReference();                                            \
      naja::SNL::SNLID::DesignObjectReference                                                   \
        snlRef(ref.getDbID(), ref.getLibraryID(), ref.getDesignID(), ref.getDesignObjectID());  \
      auto object = universe->get##TYPE(snlRef);                                                \
      if (object) {                                                                             \
        ::capnp::MallocMessageBuilder message;                                                  \
        TYPE##Info::Builder info = message.initRoot<TYPE##Info>();                              \
        if (auto busObject = dynamic_cast<naja::SNL::SNLBus##TYPE*>(object)) {                  \
          info.set##TYPE(kj::heap<SNL##BUS_TYPE##Impl>(busObject));                             \
          info.setIsBus(true);                                                                  \
        } else {                                                                                \
          auto scalarObject = static_cast<naja::SNL::SNLScalar##TYPE*>(object); \
          info.set##TYPE(kj::heap<SNL##SCALAR_TYPE##Impl>(scalarObject));                       \
          info.setIsBus(false);                                                                 \
        }                                                                                       \
        context.getResults().set##TYPE##Info(info);                                             \
      } else { \
        context.getResults().setError( \
          "No " + std::string(#TYPE) + " can be found for reference: " + snlRef.getString()); \
      }                                                                                         \
    }                                                                                           \
    return kj::READY_NOW;                                                                       \
  }

#define DESCRIBE_OBJECT(DESCRIPTION) \
  kj::Promise<void> getObject##DESCRIPTION(GetObject##DESCRIPTION##Context context) override { \
    auto universe = naja::SNL::SNLUniverse::get(); \
      if (universe) { \
        auto id = context.getParams().getId(); \
        naja::SNL::SNLID najaSNLID = SNLIDConversion::snlIDToNajaSNLID(id); \
        switch (najaSNLID.type_) { \
          case naja::SNL::SNLID::Type::DB: { \
            auto db = universe->getDB(najaSNLID.dbID_); \
            if (db) { \
              context.getResults().set##DESCRIPTION(db->get##DESCRIPTION()); \
            } \
            break; \
          } \
          case naja::SNL::SNLID::Type::Library: { \
            auto library = universe->getLibrary(najaSNLID.dbID_, najaSNLID.libraryID_); \
            if (library) { \
              context.getResults().set##DESCRIPTION(library->get##DESCRIPTION()); \
            } \
            break; \
          } \
          case naja::SNL::SNLID::Type::Design: { \
            auto design = universe->getDesign(naja::SNL::SNLID::DesignReference(najaSNLID)); \
            if (design) { \
              context.getResults().set##DESCRIPTION(design->get##DESCRIPTION()); \
            } \
            break; \
          } \
          case naja::SNL::SNLID::Type::Net: { \
            auto net = universe->getNet(naja::SNL::SNLID::DesignObjectReference(najaSNLID)); \
            if (net) { \
              context.getResults().set##DESCRIPTION(net->get##DESCRIPTION()); \
            } \
            break; \
          } \
          case naja::SNL::SNLID::Type::NetBit: { \
            auto netBit = universe->getBusNetBit(najaSNLID); \
            if (netBit) { \
              context.getResults().set##DESCRIPTION(netBit->get##DESCRIPTION()); \
            } \
            break; \
          } \
          case naja::SNL::SNLID::Type::Term: { \
            auto term = universe->getTerm(naja::SNL::SNLID::DesignObjectReference(najaSNLID)); \
            if (term) { \
              context.getResults().set##DESCRIPTION(term->get##DESCRIPTION()); \
            } \
            break; \
          } \
          case naja::SNL::SNLID::Type::TermBit: { \
            auto termBit = universe->getBusTermBit(najaSNLID); \
            if (termBit) { \
              context.getResults().set##DESCRIPTION(termBit->get##DESCRIPTION()); \
            } \
            break; \
          } \
          case naja::SNL::SNLID::Type::Instance: { \
            auto instance = universe->getInstance(naja::SNL::SNLID::DesignObjectReference(najaSNLID)); \
            if (instance) { \
              context.getResults().set##DESCRIPTION(instance->get##DESCRIPTION()); \
            } \
            break; \
          } \
          case naja::SNL::SNLID::Type::InstTerm: { \
            auto instTerm = universe->getInstTerm(najaSNLID); \
            if (instTerm) { \
              context.getResults().set##DESCRIPTION(instTerm->get##DESCRIPTION()); \
            } \
            break; \
          } \
        } \
      } \
      return kj::READY_NOW; \
    }

class SNLCapnPServerImpl final: public SNLCapnPServer::Server {
  public:
    kj::Promise<void> getDBs(GetDBsContext context) override {
      auto universe = naja::SNL::SNLUniverse::get();
      if (universe) {
        auto dbs = context.getResults().initDbs(universe->getDBs().size());
        unsigned index = 0;
        for (auto snlDB: universe->getDBs()) {
          dbs.set(index++, kj::heap<SNLDBImpl>(snlDB));
        }
      }
      return kj::READY_NOW;
    }

    kj::Promise<void> getTopDesign(GetTopDesignContext context) override {
      auto universe = naja::SNL::SNLUniverse::get();
      if (universe) {
        context.getResults().setDesign(kj::heap<SNLDesignImpl>(universe->getTopDesign()));
      }
      return kj::READY_NOW;
    }

    kj::Promise<void> getDesign(GetDesignContext context) override {
      auto universe = naja::SNL::SNLUniverse::get();
      if (universe) {
        auto ref = context.getParams().getReference();
        naja::SNL::SNLID::DesignReference snlRef(ref.getDbID(), ref.getLibraryID(), ref.getDesignID());
        auto design = universe->getDesign(snlRef);
        if (design) {
          context.getResults().setDesign(kj::heap<SNLDesignImpl>(design));
        } else {
          context.getResults().setError(
            "No Design can be found for reference: " + snlRef.getString()
          );
        }
      }
      return kj::READY_NOW;
    }

    kj::Promise<void> getBitNet(GetBitNetContext context) override {
      auto universe = naja::SNL::SNLUniverse::get();
      if (universe) {
        auto ref = context.getParams().getReference();
        naja::SNL::SNLID::BitNetReference
          bitNetReference(
            ref.getIsBusBit(),
            ref.getDbID(),
            ref.getLibraryID(),
            ref.getDesignID(),
            ref.getDesignObjectID(),
            ref.getBit());
        auto object = universe->getBitNet(bitNetReference);
        if (object) {
          if (auto scalarNet = dynamic_cast<naja::SNL::SNLScalarNet*>(object)) {
            context.getResults().setBitNet(kj::heap<SNLBitNetImpl>(scalarNet));
          } else {
            auto busNetBit = static_cast<naja::SNL::SNLBusNetBit*>(object);
            context.getResults().setBitNet(kj::heap<SNLBusNetBitImpl>(busNetBit));
          }
        }
      }
      return kj::READY_NOW;
    }

    kj::Promise<void> getInstTerm(GetInstTermContext context) override {
      auto universe = naja::SNL::SNLUniverse::get();
      if (universe) {
        auto id = context.getParams().getId();
        auto najaSNLID = SNLIDConversion::snlIDToNajaSNLID(id);
        auto object = universe->getInstTerm(najaSNLID);
        if (object) {
          context.getResults().setInstTerm(kj::heap<SNLInstTermImpl>(object));
        } else {
          context.getResults().setError(
            "No InstTerm can be found for id: " + najaSNLID.getString()
          );
        }
      } else {
        context.getResults().setError("No Universe");
      }
      return kj::READY_NOW;
    }

    kj::Promise<void> getNetComponent(GetNetComponentContext context) override {
      auto universe = naja::SNL::SNLUniverse::get();
      if (universe) {
        auto id = context.getParams().getId();
        naja::SNL::SNLID najaSNLID = SNLIDConversion::snlIDToNajaSNLID(id);
        auto object = universe->getObject(najaSNLID);
        if (object) {
          if (auto instTerm = dynamic_cast<naja::SNL::SNLInstTerm*>(object)) {
            context.getResults().setComponent(kj::heap<SNLInstTermImpl>(instTerm));
          } else if (auto scalarTerm = dynamic_cast<naja::SNL::SNLScalarTerm*>(object)) {
            context.getResults().setComponent(kj::heap<SNLScalarTermImpl>(scalarTerm));
          } else if (auto busTermBit = dynamic_cast<naja::SNL::SNLBusTermBit*>(object)) {
            context.getResults().setComponent(kj::heap<SNLBusTermBitImpl>(busTermBit));
          } else {
            context.getResults().setError(
              "No NetComponent can be found for reference: " + najaSNLID.getString()
            );
          }
        }
      }
      return kj::READY_NOW;
    }

    GET_BUS_OBJECT(Term, ScalarTerm, BusTerm);
    GET_BUS_OBJECT(Net, Net, BusNet);
    GET_OBJECT(Instance);

    DESCRIBE_OBJECT(String);
    DESCRIBE_OBJECT(Description);
};

}

void SNLServer::start(const std::string& ip, int serverPort) {
  std::string serverAddress = ip + ":" + std::to_string(serverPort);
  // Set up a server.
  capnp::EzRpcServer server(kj::heap<SNLCapnPServerImpl>(), serverAddress);

  // Write the port number to stdout, in case it was chosen automatically.
  auto& waitScope = server.getWaitScope();
  unsigned port = server.getPort().wait(waitScope);
  if (port == 0) {
    // The address format "unix:/path/to/socket" opens a unix domain socket,
    // in which case the port will be zero.
    std::cout << "Listening on Unix socket..." << std::endl;
  } else {
    std::cout << "Listening on port " << port << "..." << std::endl;
  }

  // Run forever, accepting connections and handling requests.
  kj::NEVER_DONE.wait(waitScope);
}
