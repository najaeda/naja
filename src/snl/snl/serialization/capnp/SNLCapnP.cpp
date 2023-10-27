// Copyright The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0
//

#include "SNLCapnP.h"
#include "SNLDumpManifest.h"
#include "SNLDB.h"

using boost::asio::ip::tcp;

namespace naja { namespace SNL {

void SNLCapnP::dump(const SNLDB* db, const std::filesystem::path& path) {
  std::filesystem::create_directory(path);
  SNLDumpManifest::dump(path);
  dumpInterface(db, path/InterfaceName);
  dumpImplementation(db, path/ImplementationName);
}

void SNLCapnP::send(const SNLDB* db, const std::string& ipAddress, uint16_t port) {
  send(db, ipAddress, port, db->getID());
}

void SNLCapnP::send(const SNLDB* db, const std::string& ipAddress, uint16_t port, SNLID::DBID forceDBID) {
  boost::asio::io_service io_service;
  //socket creation
  tcp::socket socket(io_service);
  socket.connect(tcp::endpoint(boost::asio::ip::address::from_string(ipAddress), port));
  sendInterface(db, socket, forceDBID);
  sendImplementation(db, socket, forceDBID);
}

SNLDB* SNLCapnP::load(const std::filesystem::path& path) {
  loadInterface(path/InterfaceName);
  SNLDB* db = loadImplementation(path/ImplementationName);
  return db;
}

boost::asio::ip::tcp::socket SNLCapnP::getSocket(uint16_t port) {
  boost::asio::io_service io_service;
  //listen for new connection
  tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), port));
  //socket creation 
  tcp::socket socket(io_service);
  //waiting for connection
  acceptor.accept(socket);
  return std::move(socket);
}

SNLDB* SNLCapnP::receive(boost::asio::ip::tcp::socket& socket) {
  receiveInterface(socket);
  SNLDB* db = receiveImplementation(socket);
  return db;
}

SNLDB* SNLCapnP::receive(uint16_t port) {
  auto socket = getSocket(port);
  return receive(socket);
}

}} // namespace SNL // namespace naja
