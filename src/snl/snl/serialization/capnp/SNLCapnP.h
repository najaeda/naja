/*
 * Copyright 2022 The Naja Authors.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __SNL_CAPNP_H_
#define __SNL_CAPNP_H_

#include <filesystem>
#include <boost/asio.hpp>

namespace naja { namespace SNL {

class SNLDB;

class SNLCapnP {
  public:
    static boost::asio::ip::tcp::socket getSocket(uint16_t port=0); 
    static constexpr std::string_view InterfaceName = "db_interface.snl";
    static constexpr std::string_view ImplementationName = "db_implementation.snl";
    static void dump(const SNLDB* db, const std::filesystem::path& dumpPath);
    static void send(const SNLDB* db, const std::string& ipAddress, uint16_t port);
    static SNLDB* load(const std::filesystem::path& dumpPath);
    static SNLDB* receive(boost::asio::ip::tcp::socket& socket);
    static SNLDB* receive(uint16_t port);

    static void dumpInterface(const SNLDB* db, int fileDescriptor);
    static void dumpInterface(const SNLDB* db, const std::filesystem::path& interfacePath);
    static void sendInterface(const SNLDB* db, const std::string& ipAddress, uint16_t port);
    static void sendInterface(const SNLDB* db, boost::asio::ip::tcp::socket& socket); 

    static SNLDB* loadInterface(int fileDescriptor);
    static SNLDB* loadInterface(const std::filesystem::path& interfacePath);
    static SNLDB* receiveInterface(uint16_t port);
    static SNLDB* receiveInterface(boost::asio::ip::tcp::socket& socket); 

    static void dumpImplementation(const SNLDB* db, int fileDescriptor);
    static void dumpImplementation(const SNLDB* db, const std::filesystem::path& implementationPath);
    static void sendImplementation(const SNLDB* db, const std::string& ipAddress, uint16_t port);
    static void sendImplementation(const SNLDB* db, boost::asio::ip::tcp::socket& socket);

    static SNLDB* loadImplementation(int fileDescriptor);
    static SNLDB* loadImplementation(const std::filesystem::path& implementationPath);
    static SNLDB* receiveImplementation(uint16_t port);
    static SNLDB* receiveImplementation(boost::asio::ip::tcp::socket& socket); 
};

}} // namespace SNL // namespace naja

#endif // __SNL_CAPNP_H_
