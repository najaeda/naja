// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_CAPNP_H_
#define __SNL_CAPNP_H_

#include <filesystem>
//#include <boost/asio.hpp>
#include "NLID.h"

namespace naja { namespace NL {

class NLDB;

class SNLCapnP {
  public:
    //static boost::asio::ip::tcp::socket getSocket(uint16_t port=0); 
    static constexpr std::string_view InterfaceName = "db_interface.snl";
    static constexpr std::string_view ImplementationName = "db_implementation.snl";
    static void dump(const NLDB* db, const std::filesystem::path& dumpPath);
    //static void send(const NLDB* db, const std::string& ipAddress, uint16_t port);
    //static void send(const NLDB* db, const std::string& ipAddress, uint16_t port, NLID::DBID forceDBID);
    static NLDB* load(const std::filesystem::path& dumpPath, bool primitivesAreLoaded = false);  
    //static NLDB* receive(boost::asio::ip::tcp::socket& socket);
    //static NLDB* receive(uint16_t port);

    static void dumpInterface(const NLDB* db, int fileDescriptor);
    static void dumpInterface(const NLDB* db, int fileDescriptor, NLID::DBID forceDBID);
    static void dumpInterface(const NLDB* db, const std::filesystem::path& interfacePath);
    //static void sendInterface(const NLDB* db, const std::string& ipAddress, uint16_t port);
    //static void sendInterface(const NLDB* db, boost::asio::ip::tcp::socket& socket); 
    //static void sendInterface(const NLDB* db, boost::asio::ip::tcp::socket& socket, NLID::DBID forceDBID); 

    static NLDB* loadInterface(int fileDescriptor, bool primitivesAreLoaded = false);
    static NLDB* loadInterface(const std::filesystem::path& interfacePath, bool primitivesAreLoaded = false);
    //static NLDB* receiveInterface(uint16_t port);
    //static NLDB* receiveInterface(boost::asio::ip::tcp::socket& socket); 

    static void dumpImplementation(const NLDB* db, int fileDescriptor);
    static void dumpImplementation(const NLDB* db, int fileDescriptor, NLID::DBID forceDBID);
    static void dumpImplementation(const NLDB* db, const std::filesystem::path& implementationPath);
    //static void sendImplementation(const NLDB* db, const std::string& ipAddress, uint16_t port);
    //static void sendImplementation(const NLDB* db, boost::asio::ip::tcp::socket& socket);
    //static void sendImplementation(const NLDB* db, boost::asio::ip::tcp::socket& socket, NLID::DBID forceDBID);

    static NLDB* loadImplementation(int fileDescriptor);
    static NLDB* loadImplementation(const std::filesystem::path& implementationPath);
    //static NLDB* receiveImplementation(uint16_t port);
    //static NLDB* receiveImplementation(boost::asio::ip::tcp::socket& socket); 
};

}} // namespace NL // namespace naja

#endif // __SNL_CAPNP_H_
