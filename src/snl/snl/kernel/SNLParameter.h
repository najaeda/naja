// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_PARAMETER_H_
#define __SNL_PARAMETER_H_

#include <boost/intrusive/set.hpp>

#include "SNLName.h"
#include "NajaObject.h"

namespace naja { namespace SNL {

class SNLDesign;

class SNLParameter : public NajaObject {
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

    /**
     * \brief Create a SNLParameter.
     * \param design owner SNLDesign.
     * \param name SNLName of the parameter.
     * \param type SNLParameter::Type of the parameter.
     * \param value value of the parameter.
     * \return created SNLParameter. 
     */
    static SNLParameter* create(SNLDesign* design, const SNLName& name, Type type, const std::string& value);
    
    /// \brief Destroy this SNLParameter.
    void destroy();
    /// \return this SNLParameter name.
    SNLName getName() const { return name_; }
    std::string getValue() const { return value_; }
    Type getType() const { return type_; }
    /// \return this SNLParameter owning SNLDesign.
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
    bool deepCompare(const SNLParameter* other, std::string& reason) const;
  private:
    SNLParameter(SNLDesign* design, const SNLName& name, Type type, const std::string& value);
    static void preCreate(SNLDesign* design, const SNLName& name);
    void postCreate();
    void destroyFromDesign();

    SNLDesign*                          design_                 { nullptr };
    SNLName                             name_                   {};
    Type                                type_                   { Type::Decimal };
    std::string                         value_                  {};
    boost::intrusive::set_member_hook<> designParametersHook_   {};
};

}} // namespace SNL // namespace naja

#endif // __SNL_PARAMETER_H_