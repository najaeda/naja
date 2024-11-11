#include <string>
#include <vector>
using namespace std;

#include "imgui.h"

namespace {

struct Pin {
    int id;
    std::string name;
    ImVec2 position;
    bool isInput;
};

struct Node {
    int id;
    std::string label;
    ImVec2 position;
    std::vector<Pin> inputs;
    std::vector<Pin> outputs;
};

#if 0
struct Connection {
    int outputNodeId;
    int outputPinId;
    int inputNodeId;
    int inputPinId;
};
#endif

struct Schematic {
  using Nodes = vector<Node*>;
  Nodes nodes_;
  Nodes inputs;
  Nodes outputs;
};

}

void Schematic::addNode() {
  nodes_.push_back(new Node());
}

void Schematic::breakLoops() {
  //break loops in the graph by disabling the connection that causes the loop
  vector<bool> visited(V, false);
  vector<bool> recStack(V, false);
  for (int i = 0; i < V; i++) {
            if (!visited[i]) {
                dfs(i, visited, recStack, edgesToRemove);
            }
        }

        // Remove the identified edges
        for (auto edge : edgesToRemove) {
            auto it = find(adj[edge.first].begin(), adj[edge.first].end(), edge.second);
            if (it != adj[edge.first].end()) {
                adj[edge.first].erase(it);
            }
        }
    }
  
}

void Schematic::place() {
  for (auto node : nodes_) {
    node->position = ImVec2(0, 0);
  }

  
}