// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLCapnP.h"
#include "SNLDumpManifest.h"
#include "NLDB.h"
#include "SNLDump.h"
#include "SNLDumpException.h"
#include "NajaVersion.h"

#include <sstream>

//using boost::asio::ip::tcp;

namespace naja::NL {

namespace {

void checkManifestCompatibility(const SNLDumpManifest& manifest) {
  const auto manifestVersion = manifest.getSchemaVersion();
  const auto expectedVersion = SNLDump::getVersion();
  if (manifestVersion == expectedVersion) {
    return;
  }
  std::ostringstream reason;
  reason << "Incompatible SNL snapshot schema version: manifest has "
    << manifestVersion.getMajor() << "." << manifestVersion.getMinor()
    << "." << manifestVersion.getRevision()
    << ", reader expects "
    << expectedVersion.getMajor() << "." << expectedVersion.getMinor()
    << "." << expectedVersion.getRevision();
  if (not manifest.getProducerVersion().empty()) {
    reason << " (producer naja version " << manifest.getProducerVersion();
    if (not manifest.getProducerGitHash().empty()) {
      reason << ", git hash " << manifest.getProducerGitHash();
    }
    reason << ")";
  }
  throw SNLDumpException(reason.str());
}

void checkManifestProducer(const SNLDumpManifest& manifest) {
  if (manifest.getProducerVersion() == naja::NAJA_VERSION
      and manifest.getProducerGitHash() == naja::NAJA_GIT_HASH) {
    return;
  }
  std::ostringstream reason;
  reason << "Incompatible SNL snapshot producer: snapshot was built by naja "
    << "version "
    << (manifest.getProducerVersion().empty()
          ? "<missing>" : manifest.getProducerVersion())
    << ", git hash "
    << (manifest.getProducerGitHash().empty()
          ? "<missing>" : manifest.getProducerGitHash())
    << "; reader is naja version " << naja::NAJA_VERSION
    << ", git hash " << naja::NAJA_GIT_HASH;
  throw SNLDumpException(reason.str());
}

}

void SNLCapnP::dump(const NLDB* db, const std::filesystem::path& path) {
  std::filesystem::create_directory(path);
  SNLDumpManifest::dump(path);
  dumpInterface(db, path/InterfaceName);
  dumpImplementation(db, path/ImplementationName);
}

//Need to find a proper way to test serialization on the wire
//LCOV_EXCL_START
//void SNLCapnP::send(const NLDB* db, const std::string& ipAddress, uint16_t port) {
//  send(db, ipAddress, port, db->getID());
//}
//
//void SNLCapnP::send(const NLDB* db, const std::string& ipAddress, uint16_t port, NLID::DBID forceDBID) {
//  boost::asio::io_context ioContext;
//  //socket creation
//  tcp::socket socket(ioContext);
//  socket.connect(tcp::endpoint(boost::asio::ip::make_address(ipAddress), port));
//  sendInterface(db, socket, forceDBID);
//  sendImplementation(db, socket, forceDBID);
//}
//LCOV_EXCL_STOP

NLDB* SNLCapnP::load(
  const std::filesystem::path& path,
  LoadingConfiguration loadingConfiguration) {
  const auto manifest = SNLDumpManifest::load(path);
  checkManifestCompatibility(manifest);
  checkManifestProducer(manifest);
  loadInterface(path/InterfaceName, loadingConfiguration);
  NLDB* db = loadImplementation(path/ImplementationName, loadingConfiguration);
  return db;
}

//Need to find a proper way to test serialization on the wire
//LCOV_EXCL_START
//boost::asio::ip::tcp::socket SNLCapnP::getSocket(uint16_t port) {
//  boost::asio::io_context ioContext;
//  //listen for new connection
//  tcp::acceptor acceptor(ioContext, tcp::endpoint(tcp::v4(), port));
//  //socket creation 
//  tcp::socket socket(ioContext);
//  //waiting for connection
//  acceptor.accept(socket);
//  return std::move(socket);
//}
//
//NLDB* SNLCapnP::receive(boost::asio::ip::tcp::socket& socket) {
//  receiveInterface(socket);
//  NLDB* db = receiveImplementation(socket);
//  return db;
//}
//
//NLDB* SNLCapnP::receive(uint16_t port) {
//  auto socket = getSocket(port);
//  return receive(socket);
//}
//LCOV_EXCL_STOP

}  // namespace naja::NL
