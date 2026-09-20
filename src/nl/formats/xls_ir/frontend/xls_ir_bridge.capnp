# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

@0xeed755399356ef9e;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("naja::NL::XLSIRBridge");

# Versioned, Naja-owned output of the separately built XLS bridge.
struct BridgePayload {
  schemaVersion @0 :UInt32;
  xlsRevision   @1 :Text;
  packageName   @2 :Text;
  entities      @3 :List(Entity);
}

enum EntityKind {
  function @0;
  block    @1;
  proc     @2;
}

struct Entity {
  kind       @0 :EntityKind;
  name       @1 :Text;
  isTop      @2 :Bool;
  parameters @3 :List(Value);
  nodes      @4 :List(Node);
  result     @5 :Text;
  outputName @6 :Text;
}

struct Value {
  name @0 :Text;
  type @1 :Type;
}

struct Node {
  id       @0 :Int64;
  name     @1 :Text;
  op       @2 :Text;
  type     @3 :Type;
  operands @4 :List(Text);
  source   @5 :Text;
}

# The schema can describe all XLS type families even though the first reader
# accepts only bits. That makes unsupported types explicit instead of lossy.
struct Type {
  union {
    bits  @0 :UInt64;
    tuple @1 :List(Type);
    array @2 :ArrayType;
    token @3 :Void;
  }
}

struct ArrayType {
  size    @0 :UInt64;
  element @1 :Type;
}
