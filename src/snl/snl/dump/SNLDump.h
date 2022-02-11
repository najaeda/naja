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

#ifndef __SNL_DUMP_H_
#define __SNL_DUMP_H_

namespace naja { namespace SNL {

class SNLDump {
  public:
    class Version {
      public:
        static constexpr short major_    { 0 };
        static constexpr short minor_    { 0 };
        static constexpr short revision_ { 1 };
        static constexpr short getMajor() { return major_; }
        static constexpr short getMinor() { return minor_; }
        static constexpr short getRevision() { return revision_; }
        static std::string getString();
    };
};

}} // namespace SNL // namespace naja

#endif // __SNL_DUMP_H_