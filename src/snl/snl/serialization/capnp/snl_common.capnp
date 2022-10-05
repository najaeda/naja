@0x8180e964216c1d8e;

struct DesignReference {
  dbID      @0 : UInt8;
  libraryID @1 : UInt16;
  designID  @2 : UInt32;
}

struct PropertyValue {
  union {
    text  @0 : Text;
    bool  @1 : Bool;
  }
}

struct Property {
  name    @0 : Text;
  values  @1 : List(PropertyValue);
}