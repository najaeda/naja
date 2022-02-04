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

#ifndef __DNLDB_H_
#define __DNLDB_H_

#include <vector>

#include "DNLInstance.h"
#include "DNLEqui.h"

namespace DNL {

class DNLDB {
  /**
   * @brief DNLDB 
   * 
   * Top:
   * SNLID
   * 
   * Top Terminals:
   * vector<SNL::TermID, begin>
   * 
   * Instances:
   * vector<SNL::InstanceOccurrence>, begin, end>
   * [ SNL::InstanceOccurrence, 12, 56 ] 
   * 
   * Terminals
   * Const0 Equi == Index 0
   * Const1 Equi == Index 1
   * vector<EquiID>
   * 
   * Equis:
   * EquiType: Standard, NoDriver, Const0, Const1
   * vector<SNL::NetOccurrence, EquiType begin, end>
   * [ SNL::NetOccurrence, Const0, 34, 78]
   * If Type == Standard, first id is driver
   * 
   * Browsing through DB:
   * - Start from Top terminal
   * - get EquiID
   * - get EquiType,  
   * 
   * 
   */

  public:
    using DNLInstances = std::vector<DNLInstance>;
    //using DNLTerminals = std::vector<DNLTerm>;
    using DNLEquis = std::vector<DNLEqui>;
    using DNLEquisConnectivity = std::vector<size_t>;

    static DNLDB* create();
    void destroy();
  private:
    static void preCreate();
    void postCreate();
    void preDestroy();

    void addInstance(size_t start, size_t end);

    DNLInstances  instances_;
    //DNLTerminals  terminals_;
    DNLEquis      equis_;
};

}

#endif /* __DNL_DB_H_ */
