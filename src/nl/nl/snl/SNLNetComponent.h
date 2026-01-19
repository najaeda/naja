// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <boost/intrusive/set.hpp>

#include "SNLDesignObject.h"

namespace naja::NL {

class SNLNet;
class SNLBitNet;

class SNLNetComponent: public SNLDesignObject {
  public:
    friend class SNLBitNet;
    using super = SNLDesignObject;
 
    /**
     * \brief class describing Direction.
    */
    class Direction {
      public:
        enum DirectionEnum {
          Input,    ///< Input direction.
          Output,   ///< Output direction.
          InOut,    ///< InOut direction.
          Undefined ///< Undefined direction.
        };
        Direction(const DirectionEnum& dirEnum);
        Direction(const Direction& direction) = default;
        Direction& operator=(const Direction& direction) = default;
        operator const DirectionEnum&() const {return dirEnum_;}
        std::string getString() const;
        private:
          DirectionEnum dirEnum_;
    };

    /// \return this SNLNetComponent Direction.
    virtual Direction getDirection() const = 0;

    /// \return this SNLNetComponent SNLNet. 
    virtual SNLBitNet* getNet() const =0;

    /**
     * \brief Change this SNLNetComponent SNLNet.
     * \remark This SNLNetComponent and net must have the same size. 
     * \remark If net is null, this SNLNetComponent will be disconnected.
     */
    virtual void setNet(SNLNet* net) =0;

    virtual bool deepCompare(const SNLNetComponent* other, std::string& reason) const = 0;

  protected:
    SNLNetComponent() = default;

    static void preCreate();
    void postCreate() override;
    void preDestroy() override;

  private:
    boost::intrusive::set_member_hook<> netComponentsHook_  {};
};

}  // namespace naja::NL