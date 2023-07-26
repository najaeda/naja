// Copyright The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0
//

#include "SNLCapnP.h"
#include "SNLDumpManifest.h"

namespace naja { namespace SNL {

void SNLCapnP::dump(const SNLDB* db, const std::filesystem::path& path) {
  std::filesystem::create_directory(path);
  SNLDumpManifest::dump(path);
  dumpInterface(db, path/InterfaceName);
  dumpImplementation(db, path/ImplementationName);
}

SNLDB* SNLCapnP::load(const std::filesystem::path& path) {
  loadInterface(path/InterfaceName);
  SNLDB* db = loadImplementation(path/ImplementationName);
  return db;
}

}} // namespace SNL // namespace naja