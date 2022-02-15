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

#include "SNLFlattenerNetTreeNode.h"

namespace naja { namespace SNL {

void SNLFlattenerNetTreeNode::print(std::ostream& stream, unsigned indent) const {
  stream << std::string(indent, ' ') << getString() << std::endl;
  indent += 2;
}

std::string SNLFlattenerNetTreeNode::getString() const {
  return std::string();
}

}} // namespace SNL // namespace naja