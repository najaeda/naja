// Copyright 2025 The Naja Authors.
// SPDX-FileCopyrightText: 2025 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#if defined(_WIN32)
  #ifdef NAJA_PYTHON_EXPORTS
    #define NAJA_PY_EXPORT __declspec(dllexport)
  #else
    #define NAJA_PY_EXPORT __declspec(dllimport)
  #endif
#else
  #define NAJA_PY_EXPORT
#endif