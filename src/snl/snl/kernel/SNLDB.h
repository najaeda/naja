// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_DB_H_
#define __SNL_DB_H_

#include "SNLObject.h"
#include "SNLLibrary.h"

namespace naja { namespace SNL {

class SNLUniverse;

class SNLDB final: public SNLObject {
  public:
    friend class SNLUniverse;
    friend class SNLLibrary;
    using super = SNLObject;

    using SNLDBLibrariesHook =
      boost::intrusive::member_hook<SNLLibrary, boost::intrusive::set_member_hook<>, &SNLLibrary::dbLibrariesHook_>;
    using SNLDBLibraries = boost::intrusive::set<SNLLibrary, SNLDBLibrariesHook>;

    SNLDB(const SNLDB&) = delete;

    /**
     * \brief Create a SNLDB in the SNLUniverse universe.
     * \param universe the SNLUniverse in which the SNLDB will be created.
     * \return the created SNLDB.
     */
    static SNLDB* create(SNLUniverse* universe);
    /**
     * \brief Create a SNLDB in the SNLUniverse universe with a specific SNLID::DBID.
     * \param universe the SNLUniverse in which the SNLDB will be created.
     * \param id the SNLID::DBID of the SNLDB to create.
     * \return the created SNLDB.
     */
    static SNLDB* create(SNLUniverse* universe, SNLID::DBID id);

    /// \return the SNLID::DBID of this SNLDB.
    SNLID::DBID getID() const { return id_; }
    /// \return the SNLID of this SNLDB. 
    SNLID getSNLID() const;

    /**
     * \brief Change the SNLDB id. Main purpose: compare DBs after save and load.
     * \param id new DBID
     * \warning use with caution: all DB objects SNLIDs will be modified, as the DB id part of SNLID will
     * be modified.
     */
    void setID(SNLID::DBID id);

    /// \return the SNLLibrary in this SNLDB with SNLID::LibraryID:id 
    SNLLibrary* getLibrary(SNLID::LibraryID id) const;
    /// \return the SNLLibrary in this SNLDB with SNLName:name 
    SNLLibrary* getLibrary(const SNLName& name) const;

    /// \return the SNLDesign with SNLID::DBDesignReference reference or null if it does not exist
    SNLDesign* getDesign(const SNLID::DBDesignReference& designReference) const;

    /// \return the SNLDesign named name or null if it does not exist
    SNLDesign* getDesign(const SNLName& name) const;

    bool isTopDB() const;
    SNLDesign* getTopDesign() const;
    void setTopDesign(SNLDesign* design);

    /// \return the Libraries owned by this SNLDB
    NajaCollection<SNLLibrary*> getLibraries() const;
    /// \return the all the Libraries owned (directly or indirectly) by this SNLDB
    NajaCollection<SNLLibrary*> getGlobalLibraries() const;

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;

    void mergeAssigns();

    bool operator<(const SNLDB &rdb) const {
      return getSNLID() < rdb.getSNLID();
    }

    bool deepCompare(const SNLDB* db, std::string& reason) const;

  private:
    SNLDB() = default;
    SNLDB(SNLID::DBID id);
    static void preCreate(SNLUniverse* universe);
    static void preCreate(SNLUniverse* universe, SNLID::DBID id);
    void postCreateAndSetID(SNLUniverse* universe);
    void postCreate(SNLUniverse* universe);
    void commonPreDrestroy();
    void preDestroy() override;
    void destroyFromUniverse();

    void addLibrary(SNLLibrary* library);
    void addLibraryAndSetID(SNLLibrary* library);
    void rename(SNLLibrary* library, const SNLName& previousName);
    void removeLibrary(SNLLibrary* library);

    SNLID::DBID                         id_;
    boost::intrusive::set_member_hook<> universeDBsHook_          {};
    SNLDBLibraries                      libraries_                {};
    using LibraryNameIDMap = std::map<SNLName, SNLID::LibraryID>;
    LibraryNameIDMap                    libraryNameIDMap_         {};
    SNLDesign*                          topDesign_                {nullptr};
};

}} // namespace SNL // namespace naja

#endif // __SNL_DB_H_
