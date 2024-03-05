// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "FIFPhysicalNetlist.h"

#include <queue>
#include <iostream>
#include <fcntl.h>
#include <kj/compat/gzip.h>
#include <capnp/serialize-packed.h>

#include "PhysicalNetlist.capnp.h"

namespace naja {

using Strings = std::vector<std::string>;

void loadPhysNet(
  const PhysicalNetlist::PhysNetlist::PhysNet::Reader& net,
  const Strings& strings) {
  if (net.getType() == PhysicalNetlist::PhysNetlist::NetType::SIGNAL) {
    auto nameID = net.getName();
    if (nameID >= strings.size()) {
      std::cerr << "Invalid name ID: " << nameID << std::endl;
      std::exit(1);
    }
    auto name = strings[nameID];
    std::cerr << "Net: " << name << ", stubs: " << net.getStubs().size() << std::endl;
    uint stubID = 0;
    for (auto stub: net.getStubs()) {
      std::cerr << "  Stub: " << stubID <<  ", branches: " << stub.getBranches().size() << std::endl;
      ++stubID;
      using BranchQueue = std::queue<PhysicalNetlist::PhysNetlist::RouteBranch::Reader>;
      BranchQueue branchQueue;
      for (auto branch: stub.getBranches()) {
        branchQueue.push(branch);
      }
      while (not branchQueue.empty()) {
        auto branch = branchQueue.front();
        branchQueue.pop();
        for (auto subBranch: branch.getBranches()) {
          branchQueue.push(subBranch);
        }
        
        auto routeSegment = branch.getRouteSegment();
        if (routeSegment.hasBelPin()) {
          std::cerr << "has BelPin" << std::endl;
        }
        if (routeSegment.hasPip()) {
          std::cerr << "has pip" << std::endl;
        }
        if (routeSegment.hasSitePin()) {
          std::cerr << "has site pin" << std::endl;
        }
        if (routeSegment.hasSitePIP()) {
          std::cerr << "has site pip" << std::endl;
        }
        if (routeSegment.hasBelPin()) {
          auto belPin = routeSegment.getBelPin();
          auto belID = belPin.getBel();
          if (belID >= strings.size()) {
            std::cerr << "Invalid bel ID: " << belID << std::endl;
            std::exit(1);
          }
          auto belName = strings[belID];
          std::cerr << "bel: " << belName << std::endl;
          auto pinID = belPin.getPin();
          if (pinID >= strings.size()) {
            std::cerr << "Invalid pin ID: " << pinID << std::endl;
            std::exit(1);
          }
          auto pinName = strings[pinID];
          std::cerr << "pin: " << pinName << std::endl;

          auto siteID = belPin.getSite();
          if (siteID >= strings.size()) {
            std::cerr << "Invalid site ID: " << siteID << std::endl;
            std::exit(1);
          }
          auto siteName = strings[siteID];
          std::cerr << "site: " << siteName << std::endl;
        }
        if (routeSegment.hasSitePin()) {
          auto sitePin = routeSegment.getSitePin();
          auto sitePinNameID = sitePin.getPin();
          if (sitePinNameID >= strings.size()) {
            std::cerr << "Invalid site pin name ID: " << sitePinNameID << std::endl;
            std::exit(1);
          }
          auto sitePinName = strings[sitePinNameID];
          std::cerr << "site: " << sitePinName << std::endl;
        }
      }
    }
  }
}

void FIFPhysicalNetlist::load(const std::filesystem::path& path) {
   // Open the file
  int fd = open(path.c_str(), O_RDONLY);
  if (fd == -1) {
    std::cerr << "Failed to open " << path << std::endl;
    std::exit(1);
  }

  // Create a file descriptor input stream
  kj::FdInputStream fdInputStream(fd);
  // Wrap the input stream with gzip decompression
  kj::GzipInputStream gzipInputStream(fdInputStream);

  // Now, create a buffered input stream wrapper around the gzip input stream
  kj::BufferedInputStreamWrapper bufferedInputStream(gzipInputStream);

  // Use the buffered input stream with Cap'n Proto
  capnp::ReaderOptions options;
  options.traversalLimitInWords = std::numeric_limits<uint64_t>::max();
  capnp::InputStreamMessageReader messageReader(bufferedInputStream, options);

  PhysicalNetlist::PhysNetlist::Reader netlist = messageReader.getRoot<PhysicalNetlist::PhysNetlist>();

  Strings strings;
  for (auto str: netlist.getStrList()) {
    strings.push_back(str);
  }
  for (auto net: netlist.getPhysNets()) {
    loadPhysNet(net, strings);
  }
}

} // namespace naja