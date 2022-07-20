@0xc1f73089d4df5f68;

struct DBInterface {
  id                @0 : UInt8 = 1;
  libraryInterfaces @1 : List(LibraryInterface);

  enum LibraryType {
    standard    @0;
    primitives  @1;
  }

  struct LibraryInterface {
    id                @0 : UInt8 = 0;
    name              @1 : Text;
    type              @2 : LibraryType; # = Direction::Standard;
    libraryInterfaces @3 : List(LibraryInterface);
    designInterfaces  @4 : List(DesignInterface);

    enum DesignType {
      standard  @0;
      blackbox  @1;
      primitive @2;
    }
    
    struct DesignInterface {
      id          @0 : UInt32 = 0;
      name        @1 : Text;
      type        @2 : DesignType; # = Standard
      parameters  @3 : List(Parameter);
      terms       @4 : List(Term);

      struct Parameter {
        name  @0 : Text;
        value @1 : Text;
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