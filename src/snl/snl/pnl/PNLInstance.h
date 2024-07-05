// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PNL_INSTANCE_H_
#define __PNL_INSTANCE_H_

#include "PNLDesignObject.h"
//#include "PNLTransform.h"

namespace naja { namespace SNL {

class PNLInstance final: public PNLDesignObject {
  public:
    using super = PNLDesignObject;

    static PNLInstance* create(PNLDesign* design, PNLDesign* model);

    PNLDesign* getDesign() const override { return design_; }
    PNLDesign* getModel() const { return model_; }

    const char* getTypeName() const override { return "PNLInstance"; }
    std::string getString() const override { return "PNLInstance"; }
    std::string getDescription() const override { return "PNLInstance"; }
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;
  private:
    PNLInstance(PNLDesign* design, PNLDesign* model);

    static void preCreate(PNLDesign* design, const PNLDesign* model);
    void postCreate();
  
    PNLDesign*    design_;
    PNLDesign*    model_;
    //PNLPoint      origin_;
    //PNLTransform  transform_;
};

}} // namespace SNL // namespace naja

#endif // __PNL_INSTANCE_H_