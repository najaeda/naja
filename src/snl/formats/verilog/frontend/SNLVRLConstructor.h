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

#ifndef __SNL_VRL_CONSTRUCTOR_H_
#define __SNL_VRL_CONSTRUCTOR_H_

#include "VerilogConstructor.h"

namespace naja { namespace SNL {

class SNLLibrary;
class SNLDesign;

class SNLVRLConstructor: public naja::verilog::VerilogConstructor {
  public:
    SNLVRLConstructor() = delete;
    SNLVRLConstructor(const SNLVRLConstructor&) = delete;
    SNLVRLConstructor(SNLLibrary* library);

    bool inFirstPass() const { return firstPass_; }
    void setFirstPass(bool mode) { firstPass_ = mode; }
    void startModule(std::string&& name) override;
    void moduleInterfaceCompletePort(naja::verilog::Port&& port) override;
    void addNet(naja::verilog::Net&& net) override;
    void startInstantiation(std::string&& modelName) override;
    void addInstance(std::string&& name) override;
  private:
    bool        firstPass_        {true};
    SNLLibrary* library_          {nullptr};
    SNLDesign*  currentModule_    {nullptr};
    std::string currentModelName_ {};
};

}} // namespace SNL // namespace naja

#endif // __NAJA_VERILOG_CONSTRUCTOR_H_
