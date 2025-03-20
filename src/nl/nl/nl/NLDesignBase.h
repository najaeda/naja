#ifndef __NL_DESIGN_BASE_H_
#define __NL_DESIGN_BASE_H_

#include "NLDB.h"

namespace naja { namespace NL {

template <typename Derived>
class NLDesignBase: public NLObject {
  public:
    friend class NLLibrary;

    ///\return owning NLDB
    NLDB* getDB() const { return getLibrary().getDB(); }
    /// \return owning NLLibrary.
    NLLibrary* getLibrary() const { return library_; }
    NLID::DesignID getID() const { return id_; }
    NLID getNLID() const;

    friend bool operator< (const Derived& ld, const Derived& rd) {
      return ld.getNLID() < rd.getNLID();
    }

  protected:
};

}} // namespace NL // namespace naja

#endif // __NL_DESIGN_BASE_H_
