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

#include "NajaDumpableProperty.h"
#include "NajaObject.h"
#include "NajaException.h"

namespace naja {

NajaDumpableProperty::NajaDumpableProperty(const std::string& name):
  super(),
  name_(name)
{}

NajaDumpableProperty* NajaDumpableProperty::create(NajaObject* owner, const std::string& name) {
  super::preCreate(owner, name);
  NajaDumpableProperty* property = new NajaDumpableProperty(name);
  property->postCreate(owner);
  return property;
}

//LCOV_EXCL_START
std::string NajaDumpableProperty::getString() const {
  return std::string();
}
//LCOV_EXCL_STOP

} // namespace naja