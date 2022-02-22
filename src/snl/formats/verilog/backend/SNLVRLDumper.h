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
#include <vector>
#include <map>
#include <set>

#include "SNLName.h"
#include "SNLID.h"

namespace naja { namespace SNL {

class SNLDesign;
class SNLInstance;
class SNLTerm;
class SNLNet;
class SNLBitNet;
class SNLBusNetBit;

class SNLVRLDumper {
  public:
    class Configuration {
    };
    void dumpDesign(const SNLDesign* design, std::ostream& o);
  private:
    struct DesignAnonymousNaming {
      using TermNames = std::map<SNLID::DesignObjectID, std::string>;
      std::string name_;
      TermNames   termNames_;
    };
    using DesignsAnonynousNaming = std::map<SNLID, DesignAnonymousNaming>;
    struct DesignInsideAnonymousNaming {
      using InstanceNames = std::map<SNLID::DesignObjectID, std::string>;
      using InstanceNameSet = std::set<std::string>;
      using NetNames = std::map<SNLID::DesignObjectID, std::string>;
      using NetNameSet = std::set<std::string>;
      InstanceNames   instanceNames_    {};
      InstanceNameSet instanceNameSet_  {};
      NetNames        netNames_         {};
      NetNameSet      netNameSet_       {};
    };
    static std::string createDesignName(const SNLDesign* design);
    static std::string createInstanceName(const SNLInstance* instance, DesignInsideAnonymousNaming& naming);
    static std::string createNetName(const SNLNet* net, DesignInsideAnonymousNaming& naming);
    void dumpOneDesign(const SNLDesign* design, std::ostream& o);
    void dumpInstances(const SNLDesign* design, std::ostream& o, DesignInsideAnonymousNaming& naming);
    void dumpInstance(const SNLInstance* instance, std::ostream& o, DesignInsideAnonymousNaming& naming);
    void dumpInstanceInterface(const SNLInstance* instance, std::ostream& o);
    void dumpNets(const SNLDesign* design, std::ostream& o, DesignInsideAnonymousNaming& naming);
    void dumpNet(const SNLNet* net, std::ostream& o, DesignInsideAnonymousNaming& naming);
    void dumpInterface(const SNLDesign* design, std::ostream& o, DesignInsideAnonymousNaming& naming);
    using BitNetVector = std::vector<SNLBitNet*>;
    void dumpInsTermConnectivity(const SNLTerm* term, BitNetVector& termNets, std::ostream& o);
    using ContiguousNetBits = std::vector<SNLBusNetBit*>;
    void dumpRange(const ContiguousNetBits& bits, std::ostream& o);

    DesignsAnonynousNaming designsAnonymousNaming_ {};
};

}} // namespace SNL // namespace naja

#endif // __SNL_VRL_DUMPER_H_
