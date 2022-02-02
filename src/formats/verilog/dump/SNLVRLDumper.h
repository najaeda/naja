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

#ifndef __SNL_VRL_DUMPER_H_
#define __SNL_VRL_DUMPER_H_

#include <filesystem>

#include "SNLName.h"

namespace SNL {

class SNLDesign;
class SNLTerm;
class SNLNet;
class SNLInstance;

class SNLVRLDumper {
  public:
    class Configuration {
    };
    void dumpDesign(const SNLDesign* design, std::ostream& o);
  private:
    static std::string createDesignName(const SNLDesign* design);
    static std::string createInstanceName(const SNLInstance* instance);
    void dumpInstances(const SNLDesign* design, std::ostream& o);
    void dumpInstance(const SNLInstance* instance, std::ostream& o);
    void dumpInstanceInterface(const SNLInstance* instance, std::ostream& o);
    void dumpNets(const SNLDesign* design, std::ostream& o);
    void dumpInterface(const SNLDesign* design, std::ostream& o);
};

}

#endif /* __SNL_VRL_DUMPER_H_ */
