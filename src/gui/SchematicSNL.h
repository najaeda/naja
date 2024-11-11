#ifndef __SCHEMATIC_SNL_H_
#define __SCHEMATIC_SNL_H_

namespace naja {
  namespace SNL {
    class SNLDesign;
  }
}

class Schematic;

class SchematicSNL {
  public:
    static Schematic* constructFromSNLDesign(const naja::SNL::SNLDesign* design);
};

#endif /* __SCHEMATIC_SNL_H_ */