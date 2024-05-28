# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

@0x8180e964216c1d8e;

struct DesignReference {
  dbID      @0 : UInt8;
  libraryID @1 : UInt16;
  designID  @2 : UInt32;
}

struct PropertyValue {
  union {
    text    @0 : Text;
    bool    @1 : Bool;
    uint64  @2 : UInt64;
  }
}

struct Property {
  name    @0 : Text;
  values  @1 : List(PropertyValue);
}