// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "FIFPhysicalNetlist.h"

int main(int argc, char** argv) {
  naja::FIFPhysicalNetlist::load(argv[1]);
  return 0;
}