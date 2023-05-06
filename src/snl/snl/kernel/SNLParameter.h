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

#ifndef __SNL_PARAMETER_H_
#define __SNL_PARAMETER_H_

#include <boost/intrusive/set.hpp>

#include "SNLName.h"

namespace naja { namespace SNL {

class SNLDesign;

class SNLParameter {
  public:
    friend class SNLDesign;

    class Type {
      public:
        enum TypeEnum {
          Decimal, Binary, Boolean, String
        };
        Type(const TypeEnum& typeEnum);
        Type(const Type&) = default;
        Type& operator=(const Type&) = default;

        operator const TypeEnum&() const { return typeEnum_; }
        std::string getString() const;
      private:
        TypeEnum typeEnum_;
    };

    SNLParameter() = delete;
    SNLParameter(const SNLParameter&) = delete;

    static SNLParameter* create(SNLDesign* design, const SNLName& name, Type type, const std::string& value);
    void destroy();
    SNLName getName() const { return name_; }
    std::string getValue() const { return value_; }
    Type getType() const { return type_; }
    SNLDesign* getDesign() const { return design_; }

    const char* getTypeName() const;
    std::string getString() const;
    std::string getDescription() const;

    friend bool operator< (const SNLParameter& lp, const SNLParameter& rp) {
      return lp.getName() < rp.getName();
    }
    struct SNLParameterComp {
      bool operator()(const SNLName& ln, const SNLParameter& rp) const {
        return ln < rp.getName();
      }
      bool operator()(const SNLParameter& lp, const SNLName& rn) const {
        return lp.getName() < rn;
      }
    };
  private:
    SNLParameter(SNLDesign* design, const SNLName& name, Type type, const std::string& value);
    static void preCreate(SNLDesign* design, const SNLName& name);
    void postCreate();
    void destroyFromDesign();

    SNLDesign*                          design_                 {nullptr};
    SNLName                             name_                   {};
    Type                                type_                   { Type::Decimal };
    std::string                         value_                  {};
    boost::intrusive::set_member_hook<> designParametersHook_   {};
};

}} // namespace SNL // namespace naja

#endif // __SNL_PARAMETER_H_