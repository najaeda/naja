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

#include "SNLDump.h"

namespace naja { namespace SNL {

const SNLDump::Version SNLDump::version_ = SNLDump::Version(0, 1, 0);

std::string SNLDump::Version::getString() {
  return std::to_string(getMajor())
    + "." + std::to_string(getMinor())
    + "." + std::to_string(getRevision());
}

}} // namespace SNL // namespace naja