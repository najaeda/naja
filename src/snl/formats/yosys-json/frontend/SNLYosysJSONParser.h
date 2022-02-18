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

#include <istream>
#include <filesystem>

#ifndef __SNL_YOSYS_JSON_PARSER_H_
#define __SNL_YOSYS_JSON_PARSER_H_

namespace naja { namespace SNL {

class SNLLibrary;

class SNLYosysJSONParser {
  public:
    static void parse(const std::filesystem::path& inputPath, SNLLibrary* library);
    static void parse(std::istream& input, SNLLibrary* library);
};

}} // namespace SNL // namespace naja

#endif //__SNL_YOSYS_JSON_PARSER_H_ 