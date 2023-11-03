// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNLID_CONVERSION_H_
#define __SNLID_CONVERSION_H_

#include "SNLID.h"
#include "snl_rpc.capnp.h"

class SNLIDConversion {
  public:
    static void najaSNLIDToSNLID(const naja::SNL::SNLID& najaSNLID, SNLID::Builder& snlid);
    static naja::SNL::SNLID snlIDToNajaSNLID(const SNLID::Reader& snlid);
};

#endif /* __SNLID_CONVERSION_H_ */
