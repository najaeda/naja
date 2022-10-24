/*
 * Copyright 2022 The Naja Authors.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __SNL_INSTTERM_H_
#define __SNL_INSTTERM_H_

#include "SNLTerm.h"

namespace naja { namespace SNL {

class SNLInstance;
class SNLBitTerm;

class SNLInstTerm final: public SNLNetComponent {
  public:
    friend class SNLInstance;
    using super = SNLNetComponent;
    SNLInstTerm() = delete;

    SNLDesign* getDesign() const override;

    SNLID getSNLID() const override;

    bool isAnonymous() const override;
    SNLTerm::Direction getDirection() const;
    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    SNLInstance* getInstance() const { return instance_; }
    SNLBitTerm* getTerm() const { return term_; }

    SNLBitNet* getNet() const override { return net_; }
    void setNet(SNLBitNet* net) override;

    void destroy() override;
  private:
    SNLInstTerm(SNLInstance* instance, SNLBitTerm* term);
    static SNLInstTerm* create(SNLInstance* instance, SNLBitTerm* term);
    static void preCreate(const SNLInstance* instance, const SNLBitTerm* term);
    void postCreate();
    void preDestroy() override;
    void destroyFromInstance();

    SNLInstance*  instance_;
    SNLBitTerm*   term_;
    SNLBitNet*    net_  { nullptr};
};

}} // namespace SNL // namespace naja

#endif /* __SNL_INSTTERM_H_ */
