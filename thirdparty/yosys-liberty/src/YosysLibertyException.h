// SPDX-FileCopyrightText: 2024 The Naja liberty authors <https://github.com/najaeda/naja-liberty/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __YOSYS_LIBERTY_EXCEPTION_H_
#define __YOSYS_LIBERTY_EXCEPTION_H_

namespace naja { namespace liberty {

struct YosysLibertyException: public std::exception {
  public:
    YosysLibertyException() = delete;
    YosysLibertyException(const YosysLibertyException&) = default;

    YosysLibertyException(const std::string& reason):
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

#endif // __YOSYS_LIBERTY_EXCEPTION_H_
