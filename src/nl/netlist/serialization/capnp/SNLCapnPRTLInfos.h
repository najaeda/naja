// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_CAPNP_RTL_INFOS_H_
#define __SNL_CAPNP_RTL_INFOS_H_

#include "naja_common.capnp.h"

namespace naja::NL {

class SNLRTLInfos;

// Serialize per-object RTL metadata (source location + extra key/value infos)
// into the shared NajaCommon.RTLInfos Cap'n Proto struct.
void dumpRTLInfos(::RTLInfos::Builder rtlInfosBuilder, const SNLRTLInfos* rtlInfos);

// Populate an SNLRTLInfos from a serialized NajaCommon.RTLInfos struct.
void loadRTLInfos(SNLRTLInfos* rtlInfos, const ::RTLInfos::Reader& reader);

}  // namespace naja::NL

#endif  // __SNL_CAPNP_RTL_INFOS_H_
