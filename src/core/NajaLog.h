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

#ifndef __NAJA_LOG_H_
#define __NAJA_LOG_H_

#include <string>

namespace naja { namespace core {

//Just initiating a placeholder class
//This needs heavy refinemenent in the future
class NajaLog {
  public:
    static void echo(const std::string_view& tag, const std::string_view& message);
    static void error(const std::string_view& tag, const std::string_view& message);
};


}} // namespace core // namespace naja

#endif //__NAJA_LOG_H_