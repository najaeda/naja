// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __NAJA_DUMPABLE_PROPERTY_H_
#define __NAJA_DUMPABLE_PROPERTY_H_

#include "NajaPrivateProperty.h"

#include <cstdint>
#include <vector>
#include <variant>

namespace naja {

class NajaDumpableProperty: public NajaPrivateProperty {
  public:
    using super = NajaPrivateProperty;

    enum TypeEnum { String, UInt64 };

    using Value = std::variant<std::string, uint64_t>;
    using Values = std::vector<Value>;

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

    Values getValues() const { return values_; }

    void addStringValue(const std::string& value);
    std::string getStringValue(size_t i) const;

    void addUInt64Value(uint64_t value);
    uint64_t getUInt64Value(size_t i) const;

  protected:
    NajaDumpableProperty(const std::string& name);

  private:
    std::string name_   {};
    Values      values_ {};
};

} // namespace naja

#endif /* __NAJA_DUMPABLE_PROPERTY_H_ */