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
  core::NajaUtils::createBanner(stream, "SNL manifest", "#");
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