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

#ifndef __SNL_FLATTENER_H_
#define __SNL_FLATTENER_H_

#include <map>
namespace naja { namespace SNL {

class SNLDesign;
class SNLInstance;
class SNLBitTerm;
class SNLFlattenerInstanceTree;
class SNLFlattenerInstanceTreeNode;
class SNLFlattenerNetTreeNode;
class SNLFlattenerNetForest;

class SNLFlattener {
  public:
    SNLFlattener() = default;
    SNLFlattener(const SNLFlattener&) = delete;
    SNLFlattener(SNLFlattener&&) = delete;
    ~SNLFlattener();

    void process(const SNLDesign* top);

    SNLFlattenerInstanceTree* getInstanceTree() const { return tree_; }
    SNLFlattenerNetForest* getNetForest() const { return forest_; }

  private:
    using TermNodesMap = std::map<SNLBitTerm*, SNLFlattenerNetTreeNode*>;
    void processTop(const SNLDesign* top);
    void processTopNets(const SNLDesign* top);
    void processInstance(
      SNLFlattenerInstanceTreeNode* parent,
      const SNLInstance* instance,
      const TermNodesMap& termNodesMap);
    void processDesign(
      SNLFlattenerInstanceTreeNode* node,
      const SNLDesign* model,
      const TermNodesMap& termNodesMap);

    SNLFlattenerInstanceTree* tree_   { nullptr };
    SNLFlattenerNetForest*    forest_ { nullptr };
};

}} // namespace SNL // namespace naja

#endif // __SNL_FLATTENER_H_
