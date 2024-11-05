// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NajaUtils.h"

#include <chrono>
#include <string>
#include "NajaVersion.h"

namespace {

std::string currentDate() {
  auto current = std::chrono::system_clock::now();
  std::time_t currentTime = std::chrono::system_clock::to_time_t(current);
  return std::ctime(&currentTime);
}

}

namespace naja {

void NajaUtils::createBanner(
  std::ostream& stream,
  const std::string& title,
  const std::string& commentChar) {
  for (size_t i=0; i<(80/commentChar.size()); ++i) {
    stream << commentChar;
  }
  stream << std::endl;
  stream << commentChar << " " << currentDate();
  stream << commentChar << " " << title << std::endl;
  stream << commentChar << " naja version: " << naja::NAJA_VERSION << std::endl;
  stream << commentChar << " Git hash: " << naja::NAJA_GIT_HASH << std::endl;
  for (size_t i=0; i<(80/commentChar.size()); ++i) {
    stream << commentChar;
  }
  stream << std::endl;
}

} // namespace naja