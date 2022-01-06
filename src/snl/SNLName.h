#ifndef __SNL_NAME_H_
#define __SNL_NAME_H_

#include <string>

namespace SNL {

class SNLName {
  public:
    SNLName(const std::string& name = std::string()):
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
  private:
    std::string string_;
};

}

#endif /* __SNL_NAME_H_ */
