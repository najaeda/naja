// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_DUMP_H_
#define __SNL_DUMP_H_

#include <string>

namespace naja { namespace SNL {

class SNLDesign;

class SNLDump {
  public:
    struct Version {
      Version() = delete;
      Version(const Version&) = default;
      Version(unsigned major, unsigned minor, unsigned revision):
        major_(major), minor_(minor), revision_(revision)
      {}
      bool operator==(const Version& version) const {
        return major_ == version.major_
          and minor_ == version.minor_
          and revision_ == version.revision_;
      }

      unsigned major_;
      unsigned minor_;
      unsigned revision_;
      unsigned getMajor() const    { return major_; }
      unsigned getMinor() const    { return minor_; }
      unsigned getRevision() const { return revision_; }
      std::string getString();
    };

    static const Version  version_;
    static Version getVersion() { return version_; }
    //static constexpr std::string_view DesignDBName = "design.db";
};

}} // namespace SNL // namespace naja

#endif // __SNL_DUMP_H_