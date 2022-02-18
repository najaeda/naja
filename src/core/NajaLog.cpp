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

#include "NajaLog.h"

#include <iostream>
#include <iomanip>

namespace naja { namespace core {

void NajaLog::echo(const std::string_view& tag, const std::string_view& message) {
  std::cout << std::setw(10) << tag << ':';
  std::cout << message << std::endl;
}

void NajaLog::error(const std::string_view& tag, const std::string_view& message) {
  std::cerr << std::setw(10) << tag << ':';
  std::cerr << message << std::endl;
}


}} // namespace core // namespace naja