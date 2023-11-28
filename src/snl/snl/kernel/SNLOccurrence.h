// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_OCCURRENCE_H_
#define __SNL_OCCURRENCE_H_

#include <string>

namespace naja { namespace SNL {

class SNLPath;
class SNLSharedPath;
class SNLDesignObject;

/**
 * \brief SNLOccurrence is the representation of a SNLDesignObject in a specific hierarchical context.
*/
class SNLOccurrence {
  public:
    //needs to be revised in case of refcount on sharedpath
    SNLOccurrence()=default;
    //needs to be revised in case of refcount on sharedpath
    SNLOccurrence(const SNLOccurrence&)=default;

    /**
     * \brief SNLOccurrence constructor with an empty path.
     * \param object referenced SNLDesignObject. 
     */
    SNLOccurrence(SNLDesignObject* object);

    /**
     * \brief SNLOccurrence constructor.
     * \param path SNLPath to the referenced SNLDesignObject. 
     * \param object referenced SNLDesignObject. 
     */
    SNLOccurrence(const SNLPath& path, SNLDesignObject* object);

    /// \return this SNLOccurrence path.
    SNLPath getPath() const;
    /// \return this SNLOccurrence referenced SNLDesignObject.
    SNLDesignObject* getObject() const { return object_; }
    /// \return true if this SNLOccurrence is valid.
    bool isValid() const { return object_ != nullptr; }

    bool operator==(const SNLOccurrence& occurrence) const;
    bool operator<(const SNLOccurrence& occurrence) const;

#if 0
    // Following methods can be removed if SNLOccurrence inherits
    // from SNLObject in the future.

    ///\return a string describing the object type
    virtual const char* getTypeName() const = 0;
    ///\return a simple string describing the object. Usually object name.
    virtual std::string getString() const = 0;
    ///\return a string extensively describing the object. Useful for debug.
    virtual std::string getDescription() const = 0;
#endif

  protected:
    SNLOccurrence(const SNLPath& path);
    
  private:
    SNLSharedPath*      path_   {nullptr};
    SNLDesignObject*    object_ {nullptr};
};

}} // namespace SNL // namespace naja

#endif // __SNL_OCCURRENCE_H_
