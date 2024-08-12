// SPDX-FileCopyrightText: 2024 The Naja liberty authors <https://github.com/najaeda/naja-liberty/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __LIBERTY_CONSTRUCTOR_H_
#define __LIBERTY_CONSTRUCTOR_H_

#include <vector>
#include <filesystem>
#include <istream>

namespace Yosys {
  struct LibertyAst;
}

namespace naja { namespace liberty {

class LibertyScanner;
class LibertyParser;
  
class LibertyConstructor {
  friend class LibertyParser;
  public:
    LibertyConstructor() = default;
    ~LibertyConstructor();
    using Paths = std::vector<std::filesystem::path>;
    void parse(const Paths& paths);
    Yosys::LibertyAst* parse(const std::filesystem::path& path);

    //LCOV_EXCL_START
    std::string getCurrentPath() const { return currentPath_; }
    struct Location {
      std::filesystem::path currentPath_  {};
      unsigned              line_         {0};
      unsigned              column_       {0};

      Location() = delete;
      Location(const Location&) = default;
      Location(
        const std::filesystem::path& currentPath,
        unsigned line,
        unsigned column):
        currentPath_(currentPath),
        line_(line),
        column_(column)
      {}
    };
    Location getCurrentLocation() const;
    //LCOV_EXCL_STOP
    void setCurrentLocation(unsigned line, unsigned column) {
      line_= line; column_ = column;
    }
    
  private:
    Yosys::LibertyAst* internalParse(std::istream& stream);

    std::filesystem::path currentPath_  {};
    unsigned              line_         {0};
    unsigned              column_       {0};
};

}} // namespace liberty // namespace naja

#endif /* __LIBERTY_CONSTRUCTOR_H_ */
