#include "NajaUtils.h"

#include <chrono>

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
  stream << commentChar << " " << currentDate();
  stream << commentChar << " " << title << std::endl;
}

} // namespace naja