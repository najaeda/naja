// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __NAJA_DUMPABLE_PROPERTY_H_
#define __NAJA_DUMPABLE_PROPERTY_H_

#include "NajaPrivateProperty.h"

#include <variant>

namespace naja {

class NajaDumpableProperty: public NajaPrivateProperty {
  public:
    using super = NajaPrivateProperty;

    class Type {
      public:
        enum TypeEnum {
          String, UInt64
        };
        Type(const TypeEnum& dirEnum);
        Type(const Type& type) = default;
        Type& operator=(const Type& type) = default;
        operator const TypeEnum&() const {return typeEnum_;}
        std::string getString() const;
        private:
          TypeEnum  typeEnum_;
    };

    using Value = std::variant<std::string, uint64_t>;

    NajaDumpableProperty() = delete;

    /**
     * \brief Create a NajaDumpableProperty.
     * \param owner the owner of this NajaDumpableProperty.
     * \param name the name of this NajaDumpableProperty. 
     * \return created NajaDumpableProperty.
     */ 
    static NajaDumpableProperty* create(NajaObject* owner, const std::string& name);

    Type getType() const { return type_; }

    std::string getName() const override { return name_; }
    std::string getString() const override;

    void setStringValue(const std::string& value);
    std::string getStringValue() const;

    void setUInt64Value(uint64_t value);
    uint64_t getUInt64Value() const;

  protected:
    NajaDumpableProperty(const std::string& name);
    NajaDumpableProperty(const std::string& name, const std::string& value);
    NajaDumpableProperty(const std::string& name, uint64_t value);

  private:
    std::string name_   {};
    Type        type_   { Type::String };
    Value       value_  {};
};

} // namespace naja

#endif /* __NAJA_DUMPABLE_PROPERTY_H_ */