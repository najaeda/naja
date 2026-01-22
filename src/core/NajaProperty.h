// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <string>

namespace naja {

class NajaObject;

class NajaProperty {
  public:
    friend class NajaObject;

    /// \return the name of this NajaProperty.
    virtual std::string getName() const =0;
    /// \return a string describing this NajaProperty.
    virtual std::string getString() const =0;

    /// \brief destroy this NajaProperty.
    void destroy();
  protected:
    NajaProperty() = default;
    virtual ~NajaProperty() = default;
    static void preCreate() {}
    void postCreate() {}
    virtual void preDestroy();

    virtual void onCapturedBy(NajaObject* object) =0;
    virtual void onReleasedBy(const NajaObject* object) =0;
};

} // namespace naja

