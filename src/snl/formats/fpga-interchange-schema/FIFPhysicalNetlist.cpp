// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "FIFPhysicalNetlist.h"

#include <fcntl.h>
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <zlib.h>

#include "PhysicalNetlist.capnp.h"

namespace naja {

void loadPhysCell(const PhysicalNetlist::PhysNetlist::PhysCell::Reader& cell) {
  // TODO
}

void FIFPhysicalNetlist::load(const std::filesystem::path& path) {
  // Open the file
  int fd = open(path.c_str(), O_RDONLY);
  if (fd == -1) {
    // Handle error
    //return nullptr;
  }

  ::capnp::StreamFdMessageReader message(fd);

  PhysicalNetlist::PhysNetlist::Reader netlist = message.getRoot<PhysicalNetlist::PhysNetlist>();

  for (auto cell: netlist.getPhysCells()) {
    loadPhysCell(cell);
  }


}

} // namespace naja