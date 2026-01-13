#pragma once

#include "NLException.h"

namespace naja::NL {

class SNLDesignNameConflictException : public NLException {
  public:
    SNLDesignNameConflictException(const std::string& reason):
      NLException(reason)
    {}
};

}