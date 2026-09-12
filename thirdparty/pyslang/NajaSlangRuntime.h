// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#if defined(_WIN32)
#  if defined(NAJA_SLANG_RUNTIME_EXPORTS)
#    define NAJA_SLANG_RUNTIME_API __declspec(dllexport)
#  else
#    define NAJA_SLANG_RUNTIME_API __declspec(dllimport)
#  endif
#elif defined(__GNUC__) || defined(__clang__)
#  define NAJA_SLANG_RUNTIME_API __attribute__((visibility("default")))
#else
#  define NAJA_SLANG_RUNTIME_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// Return the process-local identity of Naja's loaded slang runtime.
/// Compare this opaque token by address; never dereference or serialize it.
NAJA_SLANG_RUNTIME_API const void* NajaSlang_GetRuntimeIdentity(void);

/// Return the exact compatibility identifier for Naja's slang runtime.
NAJA_SLANG_RUNTIME_API const char* NajaSlang_GetNativeBuildID(void);

#ifdef __cplusplus
}
#endif
