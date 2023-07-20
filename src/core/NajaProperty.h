// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __NAJA_PROPERTY_H_
#define __NAJA_PROPERTY_H_

#include <string>

namespace naja {

class NajaObject;

class NajaProperty {
  public:
    friend class NajaObject;
    virtual std::string getName() const =0;
    virtual std::string getString() const =0;

    virtual bool isDumpable() const {
      return false;
    }
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

#endif /* __NAJA_PROPERTY_H_ */
