// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/// Return the process-local identity of the loaded Naja netlist runtime.
///
/// The returned address is an opaque token. Compare it by pointer equality;
/// never dereference, free, or serialize it.
const void* Naja_GetRuntimeIdentity(void);

/// Return the exact native build identifier for the loaded Naja runtime.
/// The returned string is owned by the runtime and remains valid until unload.
const char* Naja_GetNativeBuildID(void);

#ifdef __cplusplus
}
#endif

