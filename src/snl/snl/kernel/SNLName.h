// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_NAME_H_
#define __SNL_NAME_H_

#include <string>

namespace naja { namespace SNL {

class SNLName {
  public:
    explicit SNLName(const std::string& name = std::string()):
      string_(name)
    {}
    std::string getString() const {
      return string_;
    }
    bool empty() const {
      return string_.empty();
    }
    friend bool operator< (const SNLName& lname, const SNLName& rname) {
      return lname.getString() < rname.getString();
    }
    friend bool operator== (const SNLName& lname, const SNLName& rname) {
      return lname.getString() == rname.getString();
    }
    friend bool operator!= (const SNLName& lname, const SNLName& rname) {
      return lname.getString() != rname.getString();
    }
  private:
    std::string string_;
};

template<typename T>
struct SNLNameComp {
  bool operator()(const SNL::SNLName& name, const T& obj) const {
    return name < obj.getName();
  }
  bool operator()(const T& obj, const SNL::SNLName& name) const {
    return obj.getName() < name;
  }
};

}} // namespace SNL // namespace naja

#endif // __SNL_NAME_H_
