// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_INSTANCE_HEADER_H_
#define __SNL_INSTANCE_HEADER_H_

#include "SNLDesignObject.h"

namespace naja { namespace SNL {

class SNLInstanceHeader {
  public:
    SNLInstanceHeader() = delete;
    SNLInstanceHeader(const SNLInstanceHeader&) = delete;
    SNLInstanceHeader(SNLDesign* design, SNLDesign* model);
    SNLInstanceHeader(SNLDesign* design, const SNLName& modelName);

    SNLDesign* getDesign() const ;
    SNLDesign* getModel() const;
    SNLName getModelName() const;
    bool isBound() const { return model_ != nullptr; }

    const char* getTypeName() const;
    std::string getString() const;
    std::string getDescription() const;

  private:
    SNLDesign* design_;
    SNLDesign* model_;
    SNLName    modelName_;
};

}} // namespace SNL // namespace naja

#endif // __SNL_INSTANCE_HEADER_H_ 