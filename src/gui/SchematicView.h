#ifndef __SCHEMATIC_VIEW_H_
#define __SCHEMATIC_VIEW_H_

class Schematic;

class SchematicView {
  public:
    static void draw(const Schematic* schematic);
};

#endif /* __SCHEMATIC_VIEW_H_ */