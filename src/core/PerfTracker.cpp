// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PerfTracker.h"

PerfTracker::PerfTracker(const std::string& name):
  name_(name),
  start_(std::chrono::high_resolution_clock::now())
{}

PerfTracker::~PerfTracker() {
  //auto stop = std::chrono::high_resolution_clock::now();
}
