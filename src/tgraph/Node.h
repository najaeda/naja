#pragma once

namespace naja::TG {

class NodeProxy;

class Node {
  public:
    using 
    ~Node();
  private:
    NodeProxy* proxy_   {nullptr};

};

}  // namespace naja::TG