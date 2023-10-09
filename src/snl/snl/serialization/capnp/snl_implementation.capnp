# Copyright The Naja Authors.
# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0
#

@0xd0ba693613b57951;

using SNLCommon = import "snl_common.capnp";

struct DBImplementation {
  id                      @0 : UInt8 = 1;
  libraryImplementations  @1 : List(LibraryImplementation);

  struct LibraryImplementation {
    id                      @0 : UInt16 = 0;
    libraryImplementations  @1 : List(LibraryImplementation);
    designImplementations   @2 : List(DesignImplementation);

    struct DesignImplementation {
      id        @0 : UInt32 = 0;
      instances @1 : List(Instance);
      nets      @2 : List(Net);

      struct Instance {
        struct InstParameter {
          name  @0 : Text;
          value @1 : Text;
        }

        id              @0 : UInt32;
        name            @1 : Text;
        modelReference  @2 : SNLCommon.DesignReference;
        instParameters  @3 : List(InstParameter);
      }

      struct Net {
        union {
          scalarNet @0 : ScalarNet;
          busNet    @1 : BusNet;
        }
      }

      struct ScalarNet {
        id          @0 : UInt32 = 0;
        name        @1 : Text;
        components  @2 : List(NetComponentReference);
      }

      struct BusNet {
        id        @0 : UInt32 = 0;
        name      @1 : Text;
        msb       @2 : Int32; 
        lsb       @3 : Int32;
        bits      @4 : List(BusNetBit);
      }

      struct BusNetBit {
        bit         @0 : UInt32;    
        components  @1 : List(NetComponentReference);
      }

      struct NetComponentReference {
        union {
          termReference     @0 : TermReference;
          instTermReference @1 : InstTermReference;
        }
      }

      struct TermReference {
        termID  @0 : UInt32;
        bit     @1 : UInt32;    
      }

      struct InstTermReference {
        instanceID  @0 : UInt32;
        termID      @1 : UInt32;
        bit         @2 : UInt32;    
      }
    }
  }
}