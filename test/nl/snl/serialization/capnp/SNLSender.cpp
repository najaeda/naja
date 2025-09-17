// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include <fstream>

#include "SNLUniverseSnippet.h"

using namespace naja::SNL;
using path = std::filesystem::path;

int main(int argc, char* argv[]) {
  std::ofstream logFile;
  try {
    if (argc != 4) {
      exit(34);
    }
    std::string ipAddress = argv[1];
    int port = std::stoi(argv[2]);
    std::string logFileStr = argv[3];
  
    path logFilePath(logFileStr);
    logFile.open(logFilePath, std::ios::out);
  
    auto db = SNLUniverseSnippet::create();

    logFile << "Sending " << db->getString() << std::endl;
    SNLCapnP::send(db, ipAddress, port);
  } catch (const std::exception& e) {
    logFile << "Exception: " << e.what () << std::endl;
  }
  
  logFile.close();
  return 0;
}
