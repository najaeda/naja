#
# Copyright 2022 The Naja Authors.
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

@0xc1f73089d4df5f68;

using SNLCommon = import "snl_common.capnp";

struct DBInterface {
  id                  @0 : UInt8 = 1;
  properties          @1 : List(SNLCommon.Property);
  libraryInterfaces   @2 : List(LibraryInterface);
  topDesignReference  @3 : SNLCommon.DesignReference;

  enum LibraryType {
    standard    @0;
    primitives  @1;
  }

  struct LibraryInterface {
    id                @0 : UInt16 = 0;
    name              @1 : Text;
    properties        @2 : List(SNLCommon.Property);
    type              @3 : LibraryType;
    libraryInterfaces @4 : List(LibraryInterface);
    designInterfaces  @5 : List(DesignInterface);

    enum DesignType {
      standard  @0;
      blackbox  @1;
      primitive @2;
    }
    
    struct DesignInterface {
      id          @0 : UInt32 = 0;
      name        @1 : Text;
      properties  @2 : List(SNLCommon.Property);
      type        @3 : DesignType; # = Standard
      parameters  @4 : List(Parameter);
      terms       @5 : List(Term);

      enum ParameterType {
        decimal @0;
        binary  @1;
        string  @2;
      }

      struct Parameter {
        name  @0 : Text;
        type  @1 : ParameterType;
        value @2 : Text;
      }

      struct Term {
        union {
          scalarTerm  @0 : ScalarTerm;
          busTerm     @1 : BusTerm;
        }
      }

      enum Direction {
        input   @0;
        output  @1;
        inout   @2;
      }

      struct ScalarTerm {
        id        @0 : UInt32 = 0;
        name      @1 : Text;
        direction @2 : Direction;
      }

      struct BusTerm {
        id        @0 : UInt32 = 0;
        name      @1 : Text;
        direction @2 : Direction;
        msb       @3 : Int32; 
        lsb       @4 : Int32;
      }
    }
  }
}