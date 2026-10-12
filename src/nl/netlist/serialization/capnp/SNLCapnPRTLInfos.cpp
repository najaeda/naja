// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLCapnPRTLInfos.h"

#include "SNLRTLInfos.h"

namespace naja::NL {

namespace {

void dumpRange(::SourceRange::Builder builder, const SNLSourceLoc& range) {
  builder.setFile(range.file.getString());
  builder.setLine(range.line);
  builder.setEndLine(range.endLine);
  builder.setColumn(range.column);
  builder.setEndColumn(range.endColumn);
}

SNLSourceLoc loadRange(::SourceRange::Reader reader) {
  return {NLName(reader.getFile()), reader.getLine(), reader.getEndLine(),
    reader.getColumn(), reader.getEndColumn()};
}

void dumpReference(::SourceReference::Builder builder, const SNLSourceReference& reference) {
  dumpRange(builder.initRange(), reference.range);
  builder.setProvider(reference.provider.getString());
  builder.setLanguage(reference.language.getString());
  builder.setRepresentation(reference.representation.getString());
}

SNLSourceReference loadReference(::SourceReference::Reader reader) {
  return {loadRange(reader.getRange()), NLName(reader.getProvider()),
    NLName(reader.getLanguage()), NLName(reader.getRepresentation())};
}

}  // namespace

void dumpRTLInfos(::RTLInfos::Builder rtlInfosBuilder, const SNLRTLInfos* rtlInfos) {
  if (rtlInfos->hasSourceLoc()) {
    dumpRange(rtlInfosBuilder.initSourceLoc32(), *rtlInfos->getSourceLoc());
  }
  if (const auto& declaration = rtlInfos->getSourceDeclaration()) {
    dumpReference(rtlInfosBuilder.initDeclaration(), *declaration);
  }
  const auto& origins = rtlInfos->getSourceOrigins();
  if (not origins.empty()) {
    auto builders = rtlInfosBuilder.initOrigins(origins.size());
    size_t id = 0;
    for (const auto& origin: origins) {
      dumpReference(builders[id++], origin);
    }
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
  if (reader.hasSourceLoc32()) {
    rtlInfos->setSourceLoc(loadRange(reader.getSourceLoc32()));
  } else if (reader.hasSourceLoc()) {
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
  if (reader.hasDeclaration()) {
    rtlInfos->setSourceDeclaration(loadReference(reader.getDeclaration()));
  }
  for (auto origin: reader.getOrigins()) {
    rtlInfos->addSourceOrigin(loadReference(origin));
  }
  if (reader.hasInfos()) {
    for (auto info: reader.getInfos()) {
      rtlInfos->setInfo(NLName(info.getName()), info.getValue());
    }
  }
}

}  // namespace naja::NL
