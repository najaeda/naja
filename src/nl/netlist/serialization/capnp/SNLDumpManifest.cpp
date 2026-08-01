// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLDumpManifest.h"

#include <charconv>
#include <fstream>
#include <iterator>
#include <limits>
#include <sstream>
#include <system_error>
#include <vector>

#include "NajaUtils.h"
#include "NajaVersion.h"
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

[[noreturn]] void throwManifestFormatError(
    const std::filesystem::path& path,
    size_t lineNumber,
    const std::string& line,
    const std::string& problem,
    const std::string& expected) {
  std::ostringstream reason;
  reason << "Invalid SNL manifest `" << path.string() << "` at line "
         << lineNumber << ": " << problem << ".\n"
         << "  record: " << line << "\n"
         << "  expected: " << expected;
  throw naja::NL::SNLDumpException(reason.str());
}

unsigned parseVersionComponent(
    const std::filesystem::path& path,
    size_t lineNumber,
    const std::string& line,
    const std::string& token,
    std::string_view component) {
  unsigned long long parsed = 0;
  const auto* begin = token.data();
  const auto* end = begin + token.size();
  const auto result = std::from_chars(begin, end, parsed);
  if (result.ec != std::errc() || result.ptr != end ||
      parsed > std::numeric_limits<unsigned>::max()) {
    throwManifestFormatError(
      path,
      lineNumber,
      line,
      "schema " + std::string(component) +
        " must be a non-negative integer that fits in an unsigned value; got `" +
        token + "`",
      "V <major> <minor> <revision>");
  }
  return static_cast<unsigned>(parsed);
}

}

namespace naja::NL {

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
  stream << "P"
    << " " << naja::NAJA_VERSION
    << " " << naja::NAJA_GIT_HASH
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
  size_t lineNumber = 0;
  bool sawVersion = false;
  bool sawProducer = false;
  while (std::getline(stream, line)) {
    ++lineNumber;
    if (not line.empty()) {
      const auto firstNonWhitespace = line.find_first_not_of(" \t\r");
      if (firstNonWhitespace != std::string::npos &&
          line[firstNonWhitespace] == '#') {
        continue;
      }
      Tokens tokens = extractTokens(line);
      if (not tokens.empty()) {
        std::string command = tokens[0];
        if (command == "V") {
          if (tokens.size() != 4) {
            throwManifestFormatError(
              manifestPath,
              lineNumber,
              line,
              "schema version record has " + std::to_string(tokens.size() - 1) +
                " value(s)",
              "V <major> <minor> <revision>");
          }
          if (sawVersion) {
            throwManifestFormatError(
              manifestPath,
              lineNumber,
              line,
              "duplicate schema version record",
              "exactly one V <major> <minor> <revision> record");
          }
          sawVersion = true;
          manifest.version_.major_ = parseVersionComponent(
            manifestPath, lineNumber, line, tokens[1], "major version");
          manifest.version_.minor_ = parseVersionComponent(
            manifestPath, lineNumber, line, tokens[2], "minor version");
          manifest.version_.revision_ = parseVersionComponent(
            manifestPath, lineNumber, line, tokens[3], "revision");
        } else if (command == "P") {
          if (tokens.size() != 3) {
            throwManifestFormatError(
              manifestPath,
              lineNumber,
              line,
              "producer record has " + std::to_string(tokens.size() - 1) +
                " value(s)",
              "P <naja-version> <git-hash>");
          }
          if (sawProducer) {
            throwManifestFormatError(
              manifestPath,
              lineNumber,
              line,
              "duplicate producer record",
              "at most one P <naja-version> <git-hash> record");
          }
          sawProducer = true;
          manifest.producerVersion_ = tokens[1];
          manifest.producerGitHash_ = tokens[2];
        } else {
          throwManifestFormatError(
            manifestPath,
            lineNumber,
            line,
            "unknown record type `" + command + "`",
            "V <major> <minor> <revision>, P <naja-version> <git-hash>, "
            "or a comment starting with #");
        }
      }
    }
  }
  if (not sawVersion) {
    std::ostringstream reason;
    reason << "Invalid SNL manifest `" << manifestPath.string()
           << "`: missing required schema version record.\n"
           << "  expected: V <major> <minor> <revision>";
    throw SNLDumpException(reason.str());
  }
  return manifest;
}

}  // namespace naja::NL
