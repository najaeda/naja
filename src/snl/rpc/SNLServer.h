// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_SERVER_H_
#define __SNL_SERVER_H_

#include <string>

class SNLServer {
  public:
    static void start(const std::string& ip, int port);
};

#endif /* __SNL_SERVER_H_ */
