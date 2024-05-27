// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __NAJA_DUMPABLE_PROPERTY_H_
#define __NAJA_DUMPABLE_PROPERTY_H_

#include "NajaPrivateProperty.h"

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

    std::string getName() const override { return name_; }
    std::string getString() const override;

  protected:
    NajaDumpableProperty(const std::string& name);

  private:
    std::string name_ {};
};

} // namespace naja

#endif /* __NAJA_DUMPABLE_PROPERTY_H_ */