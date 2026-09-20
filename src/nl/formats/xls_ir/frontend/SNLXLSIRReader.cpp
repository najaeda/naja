// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLXLSIRReader.h"

#include <capnp/serialize-packed.h>
#include <kj/io.h>
#include <kj/std/iostream.h>

#include <fstream>
#include <limits>
#include <string>

#include "SNLXLSConstructorException.h"
#include "xls_ir_bridge.capnp.h"

namespace naja::NL {

namespace {

using XLSIRBridge::BridgePayload;
using XLSIRBridge::Entity;
using XLSIRBridge::EntityKind;
using XLSIRBridge::Type;

[[noreturn]] void failPayload(
  const std::filesystem::path& path,
  const std::string& reason) {
  throw SNLXLSConstructorException(
    "XLS bridge payload '" + path.string() + "': " + reason);
}

std::string copyText(::capnp::Text::Reader text) {
  return {text.begin(), text.size()};
}

std::string describeType(Type::Reader type) {
  switch (type.which()) {
    case Type::BITS:
      return "bits[" + std::to_string(type.getBits()) + "]";
    case Type::TUPLE:
      return "tuple";
    case Type::ARRAY:
      return "array";
    case Type::TOKEN:
      return "token";
  }
  return "unknown";
}

size_t readBitsWidth(
  const std::filesystem::path& path,
  Type::Reader type,
  const std::string& context) {
  if (type.which() != Type::BITS) {
    failPayload(
      path,
      context + " has unsupported type " + describeType(type));
  }
  const auto width = type.getBits();
  if (width == 0 || width > std::numeric_limits<size_t>::max()) {
    failPayload(
      path,
      context + " has invalid bits width " + std::to_string(width));
  }
  return static_cast<size_t>(width);
}

std::string describeKind(EntityKind kind) {
  switch (kind) {
    case EntityKind::FUNCTION:
      return "function";
    case EntityKind::BLOCK:
      return "block";
    case EntityKind::PROC:
      return "proc";
  }
  return "unknown";
}

SNLXLSIRFunction readFunction(
  const std::filesystem::path& path,
  Entity::Reader entity) {
  if (entity.getKind() != EntityKind::FUNCTION) {
    failPayload(
      path,
      "selected top '" + copyText(entity.getName()) + "' is a " +
        describeKind(entity.getKind()) + ", expected a function");
  }

  SNLXLSIRFunction function;
  function.name = copyText(entity.getName());
  function.result = copyText(entity.getResult());
  function.outputName = copyText(entity.getOutputName());

  for (auto parameter : entity.getParameters()) {
    const auto name = copyText(parameter.getName());
    function.parameters.push_back({
      name,
      readBitsWidth(path, parameter.getType(), "parameter '" + name + "'")});
  }

  for (auto node : entity.getNodes()) {
    const auto name = copyText(node.getName());
    SNLXLSIRNode loweredNode;
    loweredNode.id = node.getId();
    loweredNode.name = name;
    loweredNode.op = copyText(node.getOp());
    loweredNode.width =
      readBitsWidth(path, node.getType(), "node '" + name + "'");
    for (auto operand : node.getOperands()) {
      loweredNode.operands.push_back(copyText(operand));
    }
    loweredNode.source = copyText(node.getSource());
    function.nodes.push_back(std::move(loweredNode));
  }
  return function;
}

SNLXLSIRFunction readPayload(
  const std::filesystem::path& path,
  BridgePayload::Reader payload) {
  if (payload.getSchemaVersion() != SNLXLSIRReader::SchemaVersion) {
    failPayload(
      path,
      "incompatible schema version " +
        std::to_string(payload.getSchemaVersion()) + ", expected " +
        std::to_string(SNLXLSIRReader::SchemaVersion));
  }

  const auto revision = copyText(payload.getXlsRevision());
  if (revision != SNLXLSIRReader::XLSRevision) {
    failPayload(
      path,
      "incompatible XLS revision '" + revision + "', expected '" +
        std::string(SNLXLSIRReader::XLSRevision) + "'");
  }
  if (payload.getPackageName().size() == 0) {
    failPayload(path, "package name is empty");
  }

  Entity::Reader top;
  bool hasTop = false;
  for (auto entity : payload.getEntities()) {
    if (!entity.getIsTop()) {
      continue;
    }
    if (hasTop) {
      failPayload(path, "payload contains more than one selected top entity");
    }
    top = entity;
    hasTop = true;
  }
  if (!hasTop) {
    failPayload(path, "payload has no selected top entity");
  }
  return readFunction(path, top);
}

}  // namespace

SNLXLSIRFunction SNLXLSIRReader::load(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    failPayload(path, "cannot open file");
  }

  try {
    kj::std::StdInputStream rawInput(input);
    kj::BufferedInputStreamWrapper bufferedInput(rawInput);
    ::capnp::PackedMessageReader message(bufferedInput);
    return readPayload(path, message.getRoot<BridgePayload>());
  } catch (const SNLXLSConstructorException&) {
    throw;
  } catch (const kj::Exception& exception) {
    failPayload(path, "malformed Cap'n Proto message: " +
      std::string(exception.getDescription().cStr()));
  }
}

}  // namespace naja::NL
