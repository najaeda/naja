# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

@0x996843a4d0373104;

struct SNLID {
  enum Type {
    db @0;
    library @1;
    design @2;
    term @3;
    termbit @4;
    net @5;
    netbit @6;
    instance @7;
    instterm @8;
  }
  type            @0 :Type;
  dbID            @1 :UInt8;
  libraryID       @2 :UInt16;
  designID        @3 :UInt32;
  instanceID      @4 :UInt32;
  designObjectID  @5 :UInt32;
  bit             @6 :Int32;
}

struct SNLDesignReference {
  dbID      @0 :UInt8;
  libraryID @1 :UInt16;
  designID  @2 :UInt32;
}

struct SNLDesignObjectReference {
  dbID            @0 :UInt8;
  libraryID       @1 :UInt16;
  designID        @2 :UInt32;
  designObjectID  @3 :UInt32;
}

struct SNLBitNetReference {
  isBusBit        @0 :Bool;
  dbID            @1 :UInt8;
  libraryID       @2 :UInt16;
  designID        @3 :UInt32;
  designObjectID  @4 :UInt32;
  bit             @5 :Int32;
}

interface SNLNet {
  getTypeName @0 () -> (type :Text);
  getString @1 () -> (string :Text);
  getDescription @2 () -> (description :Text);
  getSNLID @3 () -> (snlid :SNLID);
  getID @4 () -> (id :UInt32);
  getName @5 () -> (isAnonymous :Bool, name :Text); 
  getReference @6 () -> (reference: SNLDesignObjectReference);
  getDesign @7 () -> (design :SNLDesign);
}

interface SNLBusNet extends(SNLNet) {
  getMSB @0 () -> (msb :Int32);
  getLSB @1 () -> (lsb :Int32);
  getMSBLSB @2 () -> (msb :Int32, lsb:Int32);
  getBusBits @3 () -> (busBits :List(SNLBusNetBit));
}

interface SNLBitNet extends (SNLNet) {
  getComponents @0 () -> (components :List(SNLNetComponent));
}

interface SNLBusNetBit extends (SNLBitNet) {
  getBit @0 () -> (bit: Int32);
}

enum Direction {
  input @0;
  output @1;
  inout @2;
}

interface SNLTerm {
  getTypeName @0 () -> (type :Text);
  getString @1 () -> (string :Text);
  getDescription @2 () -> (description :Text);
  getSNLID @3 () -> (snlid :SNLID);
  getName @4 () -> (isAnonymous :Bool, name :Text); 
  getReference @5 () -> (reference: SNLDesignObjectReference);
  getDirection @6 () -> (direction: Direction);
  getDesign @7 () -> (design :SNLDesign);
}

interface SNLBusTerm extends(SNLTerm) {
  getID @0 () -> (id :UInt32);
  getMSB @1 () -> (msb :Int32);
  getLSB @2 () -> (lsb :Int32);
  getMSBLSB @3 () -> (msb :Int32, lsb:Int32);
  getBusBits @4 () -> (busBits :List(SNLBusTermBit));
}

struct BitNetInfo {
  isBusBit  @0 :Bool;
  net       @1 :SNLBitNet;
}

interface SNLNetComponent {
  getTypeName @0 () -> (type :Text);
  getString @1 () -> (string :Text);
  getDescription @2 () -> (description :Text);
  getSNLID @3 () -> (snlid :SNLID);
  getName @4 () -> (isAnonymous :Bool, name :Text); 
  getReference @5 () -> (reference :SNLDesignObjectReference);
  getDirection @6 () -> (direction :Direction);
  getDesign @7 () -> (design :SNLDesign);
  getNet @8 () -> (bitNetInfo :BitNetInfo);
}

interface SNLBitTerm extends(SNLNetComponent) {
}

interface SNLScalarTerm extends(SNLBitTerm, SNLTerm) {
}

interface SNLBusTermBit extends(SNLBitTerm) {
  getBit @0 () -> (bit :Int32);
}

interface SNLInstTerm extends(SNLNetComponent) {
  getInstance @0 () -> (instance :SNLInstance);
  getTerm @1 () -> (term :SNLBitTerm);
}

struct TermInfo {
  term @0   :SNLTerm;
  isBus @1  :Bool;
}

struct NetInfo {
  net @0    :SNLNet;
  isBus @1  :Bool;
}

interface SNLInstance {
  getTypeName @0 () -> (type :Text);
  getString @1 () -> (string :Text);
  getDescription @2 () -> (description :Text);
  getSNLID @3 () -> (snlid :SNLID);
  getID @4 () -> (id :UInt32);
  getName @5 () -> (isAnonymous :Bool, name :Text);
  getReference @6 () -> (reference: SNLDesignObjectReference);
  getDesign @7 () -> (design :SNLDesign);
  getModel @8 () -> (model :SNLDesign);
  getInstTerms @9 () -> (instTerms :List(SNLInstTerm));
}
                    
interface SNLDesign  {
  getTypeName @0 ()  -> (type :Text);
  getString @1 () -> (string :Text);
  getDescription @2 () -> (description :Text);
  getSNLID @3 () ->  (snlid :SNLID);
  getID @4 () -> (id :UInt32);
  getName @5 () -> (isAnonymous :Bool, name :Text); 
  getReference @6 () -> (reference: SNLDesignReference);
  getTerms @7 () -> (terms :List(TermInfo));
  getNets @8 () -> (nets :List(NetInfo));
  getInstances @9 () -> (instances :List(SNLInstance));
  getNonPrimitiveInstances @10 () -> (nonPrimitiveInstances :List(SNLInstance));
  getPrimitiveInstances @11 () -> (primitiveInstances :List(SNLInstance));
}

interface SNLLibrary {
  getTypeName @0 () -> (type :Text);
  getString @1 () -> (string :Text);
  getDescription @2  () -> (description :Text);
  getSNLID @3 () -> (snlid :SNLID);
  getID @4 () -> (id :UInt32);
  getName @5 () -> (isAnonymous :Bool, name :Text); 
  getDesigns @6 () -> (designs :List(SNLDesign));
}

interface SNLDB {
  getTypeName @0 () -> (type :Text);
  getString @1 () -> (string :Text);
  getDescription @2  () -> (description :Text);
  getSNLID @3 () -> (snlid :SNLID);
  getID @4 () -> (id :UInt32);
  getLibraries @5 () -> (libraries :List(SNLLibrary));
  getTopDesign @6 () -> (design :SNLDesign);
}

interface SNLCapnPServer {
  getTypeName @0 () -> (type :Text);
  getString @1 () -> (string :Text);
  getDescription @2  () -> (description :Text);
  getDBs @3 () -> (dbs :List(SNLDB));
  getLibrary @4 (dbID :UInt8, libraryID :UInt16) -> (library :SNLLibrary, error :Text);
  getTopDesign @5 () -> (design :SNLDesign);
  getDesign @6 (reference :SNLDesignReference) -> (design :SNLDesign, error :Text);
  getTerm @7 (reference :SNLDesignObjectReference) -> (termInfo :TermInfo, error :Text);
  getNet @8 (reference :SNLDesignObjectReference) -> (netInfo :NetInfo, error :Text);
  getBitNet @9 (reference :SNLBitNetReference) -> (bitNet :SNLBitNet);
  getNetComponent @10 (id :SNLID) -> (component :SNLNetComponent, error :Text);
  getInstTerm @11 (id :SNLID) -> (instTerm :SNLInstTerm, error :Text);
  getInstance @12 (reference :SNLDesignObjectReference) -> (instance :SNLInstance, error :Text);
  getObjectDescription @13 (id :SNLID) -> (description :Text);
  getObjectString @14 (id :SNLID) -> (string :Text);
}
