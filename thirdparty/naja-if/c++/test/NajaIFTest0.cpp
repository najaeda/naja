// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <fcntl.h>
#include <filesystem>

#include <capnp/message.h>
#include <capnp/serialize-packed.h>

#include "snl_interface.capnp.h"

#ifndef NAJA_IF_TEST_PATH
#define NAJA_IF_TEST_PATH "Undefined"
#endif

TEST(NajaIFTest, test0) {
  std::filesystem::path outPath(NAJA_IF_TEST_PATH);
  outPath /= "NajaIFTestTest0.naja";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }

  {
    ::capnp::MallocMessageBuilder message;

    DBInterface::Builder db = message.initRoot<DBInterface>();
    db.setId(0);
    auto libraries = db.initLibraryInterfaces(2);
    auto library0InterfaceBuilder = libraries[0];
    library0InterfaceBuilder.setId(0);
    library0InterfaceBuilder.setName("LIB0");
    auto designs = library0InterfaceBuilder.initDesignInterfaces(1);
    auto design0InterfaceBuilder = designs[0];
    design0InterfaceBuilder.setId(0);
    design0InterfaceBuilder.setName("DESIGN0");
    design0InterfaceBuilder.setType(DBInterface::LibraryInterface::DesignType::STANDARD);
    auto terms = design0InterfaceBuilder.initTerms(3);
    auto term0InterfaceBuilder = terms[0];
    auto scalarTerm0 = term0InterfaceBuilder.initScalarTerm();
    scalarTerm0.setId(0);
    scalarTerm0.setName("TERM0");
    scalarTerm0.setDirection(DBInterface::LibraryInterface::DesignInterface::Direction::INPUT);
    auto term1InterfaceBuilder = terms[1];
    auto scalarTerm1 = term1InterfaceBuilder.initScalarTerm();
    scalarTerm1.setId(1);
    scalarTerm1.setName("TERM1");
    scalarTerm1.setDirection(DBInterface::LibraryInterface::DesignInterface::Direction::OUTPUT);
    auto term2InterfaceBuilder = terms[2];
    auto busTerm2 = term2InterfaceBuilder.initBusTerm();
    busTerm2.setId(2);
    busTerm2.setName("TERM2");
    busTerm2.setMsb(31);
    busTerm2.setLsb(0);
    busTerm2.setDirection(DBInterface::LibraryInterface::DesignInterface::Direction::INOUT);


    int fd = open(
      outPath.c_str(),
      O_CREAT | O_WRONLY,
      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  
    writePackedMessageToFd(fd, message);
    close(fd);
  }

  // Read the message back
  int fd = open(outPath.c_str(), O_RDONLY);
  ::capnp::PackedFdMessageReader message(fd);

  DBInterface::Reader dbInterface = message.getRoot<DBInterface>();
  auto dbID = dbInterface.getId();
  EXPECT_EQ(0, dbID);
  EXPECT_EQ(2, dbInterface.getLibraryInterfaces().size());
  auto library0Interface = dbInterface.getLibraryInterfaces()[0];
  EXPECT_EQ(0, library0Interface.getId());
  EXPECT_EQ("LIB0", library0Interface.getName());
  EXPECT_EQ(1, library0Interface.getDesignInterfaces().size());
}