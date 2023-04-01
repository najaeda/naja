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

#ifndef __SNL_UTILS_H_
#define __SNL_UTILS_H_

#include <map>
#include <vector>
#include "SNLID.h"

namespace naja { namespace SNL {

class SNLLibrary;
class SNLDesign;

class SNLUtils {
  public:
    using DesignsLevel = std::map<const SNLDesign*, unsigned>;
    static unsigned levelize(const SNLDesign* design, DesignsLevel& designsLevel);
    using DesignLevel = std::pair<const SNLDesign*, unsigned>;
    using SortedDesigns = std::vector<DesignLevel>;
    static void getDesignsSortedByHierarchicalLevel(const SNLDesign* top, SortedDesigns& sortedDesigns);
    static SNLID::Bit getSize(int msb, int lsb);
    static SNLDesign* findTop(const SNLLibrary* library);
};

}} // namespace SNL // namespace naja

#endif // __SNL_UTILS_H_
