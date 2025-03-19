// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLDumpManifest.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <iterator>

#include "NajaUtils.h"
#include "SNLDump.h"
#include "SNLDumpException.h"

namespace {

using Tokens = std::vector<std::string>;
Tokens extractTokens(const std::string& s) {
  std::stringstream ss(s);
  std::istream_iterator<std::string> begin(ss);
  std::istream_iterator<std::string> end;
  return Tokens(begin, end);
}

}

namespace naja { namespace SNL {

void SNLDumpManifest::dump(const std::filesystem::path& snlDir) {
  std::filesystem::path manifestPath(snlDir/ManifestFileName);
  std::ofstream stream;
  stream.open(manifestPath, std::ofstream::out);

  if (not stream.is_open()) {
    std::ostringstream reason;
    reason << "Cannot dump manifest as " << manifestPath.string() << " is not open";
    throw SNLDumpException(reason.str());
  }
  NajaUtils::createBanner(stream, "SNL manifest", "#");
  stream << "V"
    << " " << SNLDump::getVersion().getMajor()
    << " " << SNLDump::getVersion().getMinor()
    << " " << SNLDump::getVersion().getRevision()
    << std::endl;
}

SNLDumpManifest SNLDumpManifest::load(const std::filesystem::path& snlDir) {
  std::filesystem::path manifestPath(snlDir/"snl.mf");
  if (not std::filesystem::is_regular_file(manifestPath)) {
    std::ostringstream reason;
    reason << "Cannot load manifest as " << manifestPath.string() << " is not a regular file";
    throw SNLDumpException(reason.str());
  }
  std::ifstream stream(manifestPath);
  std::string line;
  SNLDumpManifest manifest;
  while (std::getline(stream, line)) {
    if (not line.empty()) {
      Tokens tokens = extractTokens(line);
      if (not tokens.empty()) {
        std::string command = tokens[0];
        if (command == "#") {
          continue;
        } else if (command == "V") {
          //extract version
          if (tokens.size() != 4) {
            throw SNLDumpException("Wrong formatting of version"); //LCOV_EXCL_LINE
          }
          manifest.version_.major_ = static_cast<unsigned>(std::stoi(tokens[1]));
          manifest.version_.minor_ = static_cast<unsigned>(std::stoi(tokens[2]));
          manifest.version_.revision_ = static_cast<unsigned>(std::stoi(tokens[3]));
        }
      }
    }
  }
  return manifest;
}

}} // namespace SNL // namespace naja
