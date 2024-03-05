// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __MEMORY_USAGE_H_
#define __MEMORY_USAGE_H_

class MemoryUsage {
  static void collect(double& vmUsage, double& residentSet);
};

#endif /* __MEMORY_USAGE_H_ */
