// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __NL_NAME_H_
#define __NL_NAME_H_

#include <string>

namespace naja { namespace SNL {

class NLName {
  public:
    explicit NLName(const std::string& name = std::string()):
      string_(name)
    {}

    /// \return the string representation of this NLName.
    std::string getString() const {
      return string_;
    }

    /// \return true if this NLName is empty, false otherwise.
    bool empty() const {
      return string_.empty();
    }
    friend bool operator< (const NLName& lname, const NLName& rname) {
      return lname.getString() < rname.getString();
    }
    friend bool operator> (const NLName& lname, const NLName& rname) {
      return lname.getString() > rname.getString();
    }
    friend bool operator== (const NLName& lname, const NLName& rname) {
      return lname.getString() == rname.getString();
    }
    friend bool operator!= (const NLName& lname, const NLName& rname) {
      return lname.getString() != rname.getString();
    }
  private:
    std::string string_;
};

template<typename T>
struct NLNameComp {
  bool operator()(const SNL::NLName& name, const T& obj) const {
    return name < obj.getName();
  }
  bool operator()(const T& obj, const SNL::NLName& name) const {
    return obj.getName() < name;
  }
};

}} // namespace SNL // namespace naja

#endif // __NL_NAME_H_
