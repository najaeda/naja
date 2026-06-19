// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLCapnPRTLInfos.h"

#include "SNLRTLInfos.h"

namespace naja::NL {

void dumpRTLInfos(::RTLInfos::Builder rtlInfosBuilder, const SNLRTLInfos* rtlInfos) {
  if (rtlInfos->hasSourceLoc()) {
    const auto& sourceLoc = rtlInfos->getSourceLoc();
    auto sourceLocBuilder = rtlInfosBuilder.initSourceLoc();
    sourceLocBuilder.setFile(sourceLoc->file.getString());
    sourceLocBuilder.setLine(sourceLoc->line);
    sourceLocBuilder.setEndLine(sourceLoc->endLine);
    sourceLocBuilder.setColumn(sourceLoc->column);
    sourceLocBuilder.setEndColumn(sourceLoc->endColumn);
  }
  const auto& infos = rtlInfos->getInfos();
  if (not infos.empty()) {
    auto infosBuilder = rtlInfosBuilder.initInfos(infos.size());
    size_t id = 0;
    for (const auto& info: infos) {
      auto infoBuilder = infosBuilder[id++];
      infoBuilder.setName(info.first.getString());
      infoBuilder.setValue(info.second);
    }
  }
}

void loadRTLInfos(SNLRTLInfos* rtlInfos, const ::RTLInfos::Reader& reader) {
  if (reader.hasSourceLoc()) {
    auto sourceLocReader = reader.getSourceLoc();
    SNLSourceLoc sourceLoc;
    if (sourceLocReader.hasFile()) {
      sourceLoc.file = NLName(sourceLocReader.getFile());
    }
    sourceLoc.line = sourceLocReader.getLine();
    sourceLoc.endLine = sourceLocReader.getEndLine();
    sourceLoc.column = sourceLocReader.getColumn();
    sourceLoc.endColumn = sourceLocReader.getEndColumn();
    rtlInfos->setSourceLoc(sourceLoc);
  }
  if (reader.hasInfos()) {
    for (auto info: reader.getInfos()) {
      rtlInfos->setInfo(NLName(info.getName()), info.getValue());
    }
  }
}

}  // namespace naja::NL
