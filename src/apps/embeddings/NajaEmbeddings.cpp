#include "NajaEmbeddings.h"

#include "DNL.h"
#include "SNLInstance.h"

namespace naja {

void NajaEmbeddings::generateAdjList(SNL::SNLDesign* design) {
    //Create a DNL on top of the SNL
    auto dnl = DNL::get();
    //Generate the adjacency list
    std::ofstream edgesStream;
    edgesStream.open("edges.txt", std::ios::out);
    std::ofstream edgesNameStream;
    edgesNameStream.open("edgesName.txt", std::ios::out);
    std::ofstream nodesStream;
    nodesStream.open("nodes.txt", std::ios::out);
    const DNL::DNLIsoDB& fidb = dnl->getDNLIsoDB();
    using Components = std::map<const SNLDesignObject*, size_t>;
    using Edge = std::set<const SNLDesignObject*>; //Top term or instance
    Components components;
    size_t globalComponentID = 0;
    size_t edgeID = 0;
    for (size_t i = 0; i < fidb.getNumIsos(); i++) {
      Edge edge;
      const DNL::DNLIso& dnlIso = fidb.getIsoFromIsoIDconst(i);
      const auto& drivers = dnlIso.getDrivers();
      const auto& readers = dnlIso.getReaders();
      for (auto driverID: drivers) {
        const auto& driver = dnl->getDNLTerminalFromID(driverID);
        if (driver.isTopPort()) {
          auto topTerm = driver.getSnlBitTerm();
          edge.insert(topTerm);
        } else {
          auto driverInstance = driver.getDNLInstance().getSNLInstance();
          edge.insert(driverInstance);
        }
        for (const auto& readerID: readers) {
          const auto& reader = dnl->getDNLTerminalFromID(readerID);
          if (reader.isTopPort()) {
            auto topTerm = reader.getSnlBitTerm();
            edge.insert(topTerm);
          } else {
            auto readerInstance = reader.getDNLInstance().getSNLInstance();
            edge.insert(readerInstance);
          }
          //std::cerr << "Edge: " << driver << " -> " << reader << std::endl;
        }
      }
      if (edge.size() > 1) {
        edgesStream << edgeID++ << " ";
        for (auto component: edge) {
          size_t componentID = 0;
          auto cit = components.find(component);
          if (cit == components.end()) {
            componentID = globalComponentID;
            nodesStream << component->getString() << std::endl;
            components[component] = globalComponentID++;
          } else {
            componentID = cit->second;
          }
          edgesStream << componentID << " ";
        }
        edgesStream << std::endl;
      }
      dnlIso.display();
    }
}

}  // namespace naja