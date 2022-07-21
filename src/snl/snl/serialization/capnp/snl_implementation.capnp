
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

@0xd0ba693613b57951;

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
        id              @0 : UInt32;
        name            @1 : Text;
        modelReference  @2 : ModelReference;
      }

      struct ModelReference {
        dbID      @0 : UInt8 = 1;
        libraryID @1 : UInt16 = 0;
        designID  @2 : UInt32 = 0;
      }

      struct Net {
        union {
          scalarNet @0 : ScalarNet;
          busNet    @1 : BusNet;
        }
      }

      struct ScalarNet {
        id        @0 : UInt32 = 0;
        name      @1 : Text;
      }

      struct BusNet {
        id        @0 : UInt32 = 0;
        name      @1 : Text;
        msb       @2 : Int32; 
        lsb       @3 : Int32;
      }
    }
  }
}