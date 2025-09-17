// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __NAJA_EXCEPTION_H_
#define __NAJA_EXCEPTION_H_

//#include <cpptrace/cpptrace.hpp>

namespace naja {

struct NajaException: public std::exception { // cpptrace::lazy_exception {
  public:
    NajaException() = delete;
    NajaException(const NajaException&) = default;

    NajaException(const std::string& reason):
      //cpptrace::lazy_exception(),
      std::exception(),
      reason_(reason)
    {}

    //LCOV_EXCL_START
    std::string getReason() const {
      return reason_;
    }
    //LCOV_EXCL_STOP

    //LCOV_EXCL_START
    const char* what() const noexcept override {
      return reason_.c_str();
    }
    //LCOV_EXCL_STOP

  private:
    const std::string reason_;
};

} // namespace naja

#endif // __NAJA_EXCEPTION_H_
