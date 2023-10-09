// Copyright 2023 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_INST_PARAMETER_H_
#define __SNL_INST_PARAMETER_H_

#include <boost/intrusive/set.hpp>

#include "SNLName.h"

namespace naja { namespace SNL {

class SNLInstance;
class SNLParameter;

class SNLInstParameter {
  public:
    friend class SNLInstance;

    SNLInstParameter() = delete;
    SNLInstParameter(const SNLInstParameter&) = delete;

    static SNLInstParameter* create(SNLInstance* instance, SNLParameter* parameter, const std::string& value);
    void destroy();
    SNLName getName() const;
    SNLInstance* getInstance() const { return instance_; }
    SNLParameter* getParameter() const { return parameter_; }
    std::string getValue() const { return value_; }

    const char* getTypeName() const;
    std::string getString() const;
    std::string getDescription() const;

    friend bool operator< (const SNLInstParameter& lp, const SNLInstParameter& rp) {
      return lp.getName() < rp.getName();
    }
    struct SNLInstanceParameterComp {
      bool operator()(const SNLName& ln, const SNLInstParameter& rp) const {
        return ln < rp.getName();
      }
      bool operator()(const SNLInstParameter& lp, const SNLName& rn) const {
        return lp.getName() < rn;
      }
    };
    bool deepCompare(const SNLInstParameter* other, std::string& reason) const;
  private:
    SNLInstParameter(SNLInstance* instance, SNLParameter* parameter, const std::string& value);
    static void preCreate(SNLInstance* instance, SNLParameter* parameter);
    void postCreate();
    void destroyFromInstance();

    SNLInstance*                        instance_           {nullptr};
    SNLParameter*                       parameter_          {nullptr};
    std::string                         value_              {};
    boost::intrusive::set_member_hook<> instParametersHook_ {};
};

}} // namespace SNL // namespace naja

#endif // __SNL_INST_PARAMETER_H_
