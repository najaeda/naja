// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <boost/intrusive/set.hpp>

#include "PNLDesignObject.h"
#include "NLName.h"

namespace naja::NL {

class PNLNet;
class PNLBitNet;

class PNLNetComponent: public PNLDesignObject {
  public:
    friend class PNLBitNet;
    using super = PNLDesignObject;
 
    /**
     * \brief class describing Direction.
    */
    class Direction {
      public:
        enum DirectionEnum {
          Input,  ///< Input direction.
          Output, ///< Output direction.
          InOut,   ///< InOut direction.
          Tristate, ///< Tristate direction.
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

    /// \return this PNLNetComponent Direction.
    virtual Direction getDirection() const = 0;
    //virtual void setDirection(const Direction& direction) = 0;

    /// \return this PNLNetComponent PNLNet. 
    virtual PNLBitNet* getNet() const =0;

    /**
     * \brief Change this PNLNetComponent PNLNet.
     * \remark This PNLNetComponent and net must have the same size. 
     * \remark If net is null, this PNLNetComponent will be disconnected.
     */
    virtual void setNet(PNLNet* net) = 0;

    virtual NLName getName() const = 0;

  protected:
    PNLNetComponent() = default;

    static void preCreate();
    void postCreate() override;
    void preDestroy() override;

  private:
    boost::intrusive::set_member_hook<> netComponentsHook_  {};
};

}  // namespace naja::NL