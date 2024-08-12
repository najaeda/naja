// SPDX-FileCopyrightText: 2024 The Naja liberty authors <https://github.com/najaeda/naja-liberty/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __LIBERTY_EXCEPTION_H_
#define __LIBERTY_EXCEPTION_H_

namespace naja { namespace liberty {

struct LibertyException: public std::exception {
  public:
    LibertyException() = delete;
    LibertyException(const LibertyException&) = default;

    LibertyException(const std::string& reason):
      std::exception(),
      reason_(reason)
    {}

    std::string getReason() const {
      return reason_;
    }

    //LCOV_EXCL_START
    const char* what() const noexcept override {
      return reason_.c_str();
    }
    //LCOV_EXCL_STOP

  private:
    const std::string reason_;
};

}} // namespace liberty // namespace naja

#endif // __LIBERTY_EXCEPTION_H_
