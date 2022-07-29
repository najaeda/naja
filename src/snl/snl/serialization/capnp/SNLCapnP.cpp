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

#include "SNLCapnP.h"
#include "SNLDumpManifest.h"

namespace naja { namespace SNL {

void SNLCapnP::dump(const SNLDB* db, const std::filesystem::path& path) {
  std::filesystem::create_directory(path);
  SNLDumpManifest::dump(path);
  dumpInterface(db, path/InterfaceName);
  dumpImplementation(db, path/ImplementationName);
}

SNLDB* SNLCapnP::load(const std::filesystem::path& path) {
  loadInterface(path/InterfaceName);
  SNLDB* db = loadImplementation(path/ImplementationName);
  return db;
}

}} // namespace SNL // namespace naja