// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_CAPNP_H_
#define __SNL_CAPNP_H_

#include <filesystem>
//#include <boost/asio.hpp>
#include "SNLID.h"

namespace naja { namespace SNL {

class SNLDB;

class SNLCapnP {
  public:
    //static boost::asio::ip::tcp::socket getSocket(uint16_t port=0); 
    static constexpr std::string_view InterfaceName = "db_interface.snl";
    static constexpr std::string_view ImplementationName = "db_implementation.snl";
    static void dump(const SNLDB* db, const std::filesystem::path& dumpPath);
    //static void send(const SNLDB* db, const std::string& ipAddress, uint16_t port);
    //static void send(const SNLDB* db, const std::string& ipAddress, uint16_t port, SNLID::DBID forceDBID);
    static SNLDB* load(const std::filesystem::path& dumpPath);
    //static SNLDB* receive(boost::asio::ip::tcp::socket& socket);
    //static SNLDB* receive(uint16_t port);

    static void dumpInterface(const SNLDB* db, int fileDescriptor);
    static void dumpInterface(const SNLDB* db, int fileDescriptor, SNLID::DBID forceDBID);
    static void dumpInterface(const SNLDB* db, const std::filesystem::path& interfacePath);
    //static void sendInterface(const SNLDB* db, const std::string& ipAddress, uint16_t port);
    //static void sendInterface(const SNLDB* db, boost::asio::ip::tcp::socket& socket); 
    //static void sendInterface(const SNLDB* db, boost::asio::ip::tcp::socket& socket, SNLID::DBID forceDBID); 

    static SNLDB* loadInterface(int fileDescriptor);
    static SNLDB* loadInterface(const std::filesystem::path& interfacePath);
    //static SNLDB* receiveInterface(uint16_t port);
    //static SNLDB* receiveInterface(boost::asio::ip::tcp::socket& socket); 

    static void dumpImplementation(const SNLDB* db, int fileDescriptor);
    static void dumpImplementation(const SNLDB* db, int fileDescriptor, SNLID::DBID forceDBID);
    static void dumpImplementation(const SNLDB* db, const std::filesystem::path& implementationPath);
    //static void sendImplementation(const SNLDB* db, const std::string& ipAddress, uint16_t port);
    //static void sendImplementation(const SNLDB* db, boost::asio::ip::tcp::socket& socket);
    //static void sendImplementation(const SNLDB* db, boost::asio::ip::tcp::socket& socket, SNLID::DBID forceDBID);

    static SNLDB* loadImplementation(int fileDescriptor);
    static SNLDB* loadImplementation(const std::filesystem::path& implementationPath);
    //static SNLDB* receiveImplementation(uint16_t port);
    //static SNLDB* receiveImplementation(boost::asio::ip::tcp::socket& socket); 
};

}} // namespace SNL // namespace naja

#endif // __SNL_CAPNP_H_
