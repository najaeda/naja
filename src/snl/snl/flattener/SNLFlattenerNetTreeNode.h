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

#ifndef __SNL_FLATTENER_NET_TREE_NODE_H_
#define __SNL_FLATTENER_NET_TREE_NODE_H_

#include <ostream>

namespace naja { namespace SNL {

class SNLFlattenerNetTree;

class SNLFlattenerNetTreeNode {
  public:
    SNLFlattenerNetTreeNode(const SNLFlattenerNetTreeNode&) = delete;
    SNLFlattenerNetTreeNode(const SNLFlattenerNetTreeNode&&) = delete;

    SNLFlattenerNetTree* getTree() const;

    void print(std::ostream& stream, unsigned indent=0) const;
    std::string getString() const;
  private:
    SNLFlattenerNetTreeNode() = default;
    ~SNLFlattenerNetTreeNode();
};

}} // namespace SNL // namespace naja

#endif /* __SNL_FLATTENER_NET_TREE_NODE_H_ */
