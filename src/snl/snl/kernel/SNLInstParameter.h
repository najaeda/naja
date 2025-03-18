// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_INST_PARAMETER_H_
#define __SNL_INST_PARAMETER_H_

#include <boost/intrusive/set.hpp>

#include "NLName.h"
#include "NajaObject.h"

namespace naja { namespace SNL {

class SNLInstance;
class SNLParameter;

class SNLInstParameter : public NajaObject {
  public:
    friend class SNLInstance;

    SNLInstParameter() = delete;
    SNLInstParameter(const SNLInstParameter&) = delete;

    /**
     * \brief Create a new instance parameter.
     * \param instance The instance to which the parameter belongs.
     * \param parameter The instantiated parameter.
     * \param value The value of the parameter.
     * \return The new instance parameter.
     */
    static SNLInstParameter* create(SNLInstance* instance, SNLParameter* parameter, const std::string& value);
    /// \brief Destroy this instance parameter. 
    void destroy();
    /// \return The name of the parameter.
    NLName getName() const;
    /// \return The instance to which this instance parameter belongs.
    SNLInstance* getInstance() const { return instance_; }
    /// \return The instantiated parameter.
    SNLParameter* getParameter() const { return parameter_; }
    /// \return The value of the parameter.
    std::string getValue() const { return value_; }
    void setValue(const std::string& value) { value_ = value; }

    const char* getTypeName() const;
    std::string getString() const;
    std::string getDescription() const;

    friend bool operator< (const SNLInstParameter& lp, const SNLInstParameter& rp) {
      return lp.getName() < rp.getName();
    }
    struct SNLInstanceParameterComp {
      bool operator()(const NLName& ln, const SNLInstParameter& rp) const {
        return ln < rp.getName();
      }
      bool operator()(const SNLInstParameter& lp, const NLName& rn) const {
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
