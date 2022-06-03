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

#include "SNLFlattenerNetForest.h"
#include "SNLFlattenerNetTree.h"
#include "SNLFlattenerException.h"

namespace naja { namespace SNL {

SNLFlattenerNetForest::~SNLFlattenerNetForest() {
  std::for_each(trees_.begin(), trees_.end(),
    [](SNLFlattenerNetTree* tree) {
      tree->forest_ = nullptr;
      delete tree;
    }
  );
}

void SNLFlattenerNetForest::addTree(SNLFlattenerNetTree* tree) {
  tree->id_ = nextTreeID_++;
  trees_.insert(tree);
}

void SNLFlattenerNetForest::removeTree(SNLFlattenerNetTree* tree) {
  auto it = trees_.find(tree);
  if (it == trees_.end()) {
    throw SNLFlattenerException("Error while removing tree");
  }
  trees_.erase(it);
}

SNLCollection<SNLFlattenerNetTree*> SNLFlattenerNetForest::getTrees() const {
  return SNLCollection(new SNLSTLCollection(&trees_));
}

void SNLFlattenerNetForest::print(std::ostream& stream) const {
  for (auto tree: trees_) {
    tree->print(stream);
  }
}

}} // namespace SNL // namespace naja