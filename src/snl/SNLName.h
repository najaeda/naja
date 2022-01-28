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
