#ifndef __SCHEMATIC_H_
#define __SCHEMATIC_H_

#include <vector>

class Schematic;
class Node;

struct Position {
  Position() = default;
  Position(float x, float y): x_(x), y_(y) {}
  float x_  {0.0f};
  float y_  {0.0f};
};

class Pin {
  public:
    class Direction {
      public:
        enum DirectionEnum {
          Input,
          Output
        };
        Direction(const DirectionEnum& dirEnum);
        Direction(const Direction& direction) = default;
        Direction& operator=(const Direction& direction) = default;
        operator const DirectionEnum&() const {return dirEnum_;}
        std::string getString() const;
      private:
        DirectionEnum   dirEnum_;
    };
    Pin() = delete;
    Pin(const Pin&) = delete;
    Pin& operator=(const Pin&) = delete;

    Pin(Node* node, const Direction& direction);

    Direction getDirection() const { return direction_; }
    void setPosition(const Position& position) { position_ = position; }
    const Position& getPosition() const { return position_; }
  private:
    //int id;
    Node*     node_;
    Direction direction_;
    Position  position_; // relative to node
};

class Node {
  public:
    using Pins = std::vector<Pin*>;

    Node(Schematic* schematic);
    ~Node();

    const Position& getPosition() const { return position_; }

    void layout();
  private:
    void addPin(Pin* pin);

    //int id_;
    //ImVec2 position;
    Pins      inputs_    {};
    Pins      outputs_   {};
    Position  position_;
};

class Connection {
  public:
    Connection() = default;
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    Connection(Pin* inputPin, Pin* outputPin):
      inputPin_(inputPin), outputPin_(outputPin) {}
  private:
    Pin* inputPin_;
    Pin* outputPin_;
};

class Schematic {
  public:
    friend class Node;
    Schematic() = default;

    using Nodes = std::vector<Node*>;
    const Nodes& getNodes() const { return nodes_; }
    void breakLoops();
  private:
    void addNode(Node* node);

    Nodes   nodes_  {};
};

#endif /* __SCHEMATIC_H_ */