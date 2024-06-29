// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PNL_DESIGN_H_
#define __PNL_DESIGN_H_

#include "SNLObject.h"

namespace naja { namespace SNL {

class SNLLibrary;

class PNLDesign final: public SNLObject {
  public:
    using super = SNLObject;
    static PNLDesign* create(SNLLibrary* library);

    SNLLibrary* getLibrary() const { return library_; }

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;
  private:
    PNLDesign(SNLLibrary* library);
    static void preCreate(const SNLLibrary* library);
    void postCreate();
    void preDestroy() override;

    SNL::SNLLibrary*  library_;
    //PNLPoint    origin_;
};

}} // namespace SNL // namespace naja

#endif // __PNL_DESIGN_H_