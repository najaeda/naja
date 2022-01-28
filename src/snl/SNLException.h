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

#ifndef __SNL_EXCEPTION_H_
#define __SNL_EXCEPTION_H_

namespace SNL {

struct SNLException: public std::exception {
  public:
    SNLException() = delete;
    SNLException(const SNLException&) = default;

    SNLException(const std::string& reason):
      std::exception(),
      reason_(reason)
    {}

    std::string getReason() const {
      return reason_;
    }

    const char* what() const noexcept override {
      return reason_.c_str();
    }

  private:
    const std::string reason_;
};

}

#endif /* __SNL_EXCEPTION_H_ */
