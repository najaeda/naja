// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_PATH_H_
#define __SNL_PATH_H_

#include <vector>

#include "SNLID.h"

namespace naja { namespace SNL {

class SNLDesign;
class SNLInstance;
class SNLSharedPath;

/**
 * \brief SNLPath: SNLInstance path through hierarchy.
*/
class SNLPath {
  public:
    friend class SNLOccurrence;

    /// \brief SNLPath empty constructor.
    SNLPath() = default;
    /// \brief SNLPath copy constructor.
    SNLPath(const SNLPath&) = default;
    /// \brief SNLPath constructor with an instance argument. 
    SNLPath(SNLInstance* instance);
    /**
     * \brief SNLPath constructor. Will construct a new SNLPath: [headPath, tailInstance]
     * \remark tailInstance->getDesign() must be headPath->getModel().
     */
    SNLPath(const SNLPath& headPath, SNLInstance* tailInstance);
    /**
     * \brief SNLPath constructor. Will construct a new SNLPath: [headInstance, tailPath]
     * \remark headInstance->getModel() must be tailPath->getDesign().
     */
    SNLPath(SNLInstance* headInstance, const SNLPath& tailPath);
    SNLPath(SNLSharedPath* sharedPath);

    using PathStringDescriptor = std::vector<std::string>;
    /// \brief SNLPath constructor taking a vector of strings as instance path descriptor. 
    SNLPath(const SNLDesign* top, const PathStringDescriptor& descriptor);
    using PathIDDescriptor = std::vector<SNLID::DesignObjectID>;
    /// \brief SNLPath constructor taking a vector of instance IDs as instance path descriptor. 
    SNLPath(const SNLDesign* top, const PathIDDescriptor& descriptor);

    /**
     * \return this SNLPath head instance.
     *
     * [A/B/C]->getHeadPath() will return A.
     */
    SNLInstance* getHeadInstance() const;

    /**
     * \return this SNLPath tail SNLPath.
     *
     * [A/B/C]->getTailPath() will return [B/C].
     */
    SNLPath getTailPath() const;

    /**
     * \return this SNLPath head SNLPath.
     *
     * [A/B/C]->getHeadPath() will return [A/B].
     */
    SNLPath getHeadPath() const;

    /**
     * \return this SNLPath tail SNLInstance.
     * 
     * [A/B/C]->getTailInstance() will return C.
     */
    SNLInstance* getTailInstance() const;

    /**
     * \return this SNLPath head SNLInstance design.
     *
     * [A/B/C]->getDesign() will return A->getDesign()
     * \sa SNLInstance::getDesign()
     */
    SNLDesign* getDesign() const;

    /**
     * \return this SNLPath tail SNLInstance model.
     *
     * [A/B/C]->getModel() will return C->getModel()
     * \sa SNLInstance::getModel()
     */
    SNLDesign* getModel() const;

    /// \return true if this SNLPath is empty, same as == SNLPath().
    bool empty() const;
    
    /**
     * \return this SNLPath size.
     *
     * [A/B/C].size() will return 3.
     */ 
    size_t size() const;

    /// \brief SNLPath assignment operator. 
    SNLPath& operator=(const SNLPath& path) = default;

    /// \brief SNLPath equality operator.
    bool operator==(const SNLPath& rhs) const;
    /// \brief SNLPath inequality operator.
    bool operator!=(const SNLPath& rhs) const;
    /// \brief SNLPath less operator.
    bool operator<(const SNLPath& rhs) const;

    /**
     * \return a string describing this SNLPath.
     * \param separator optional separator character with default value being '/'.
     *
     * [A/B/C].getString('#') will return "A#B#C".
     */
    std::string getString(const char separator='/') const;

  private:
    static SNLSharedPath* createInstanceSharedPath(SNLInstance* instance);
    SNLSharedPath* getSharedPath() const { return sharedPath_; }
    SNLSharedPath*  sharedPath_ {nullptr};
};

}} // namespace SNL // namespace naja

#endif // __SNL_PATH_H_
