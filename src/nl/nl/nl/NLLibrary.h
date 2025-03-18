// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __NL_LIBRARY_H_
#define __NL_LIBRARY_H_

#include <map>
#include "NajaCollection.h"
#include "SNLDesign.h"

namespace naja { namespace SNL {

class NLDB;

class NLLibrary final: public NLObject {
  public:
    friend class NLDB;
    friend class SNLDesign;
    using super = NLObject;

    class Type {
      public:
        enum TypeEnum {
          Standard, InDB0, Primitives
        };
        Type(const TypeEnum& typeEnum);
        Type(const Type& type) = default;
        operator const TypeEnum&() const {return typeEnum_;}
        std::string getString() const;
        private:
          TypeEnum typeEnum_;
    };

    NLLibrary() = delete;
    NLLibrary(const NLLibrary& library) = delete;

    /** 
     * \brief Create a NLLibrary in the NLDB db.
     * \param db the NLDB in which the NLLibrary will be created.
     * \param name optional name of the NLLibrary to create. if name is not provided,
     * the created NLLibrary will be anonymous.
     * \return the created NLLibrary.
     */
    static NLLibrary* create(NLDB* db, const NLName& name = NLName());
    /**
     * \brief Create a NLLibrary in the NLDB db with a specific Type.
     * \param db the NLDB in which the NLLibrary will be created.
     * \param type the Type of the NLLibrary to create.
     * \param name optional name of the NLLibrary to create. if name is not provided,
     * the created NLLibrary will be anonymous.
     * \return the created NLLibrary.
     */
    static NLLibrary* create(NLDB* db, Type type, const NLName& name = NLName());
    /**
     * \brief Create a NLLibrary in the NLDB db with a specific NLID::LibraryID.
     * \param db the NLDB in which the NLLibrary will be created.
     * \param id the NLID::LibraryID of the NLLibrary to create.
     * \param name optional name of the NLLibrary to create. if name is not provided,
     * the created NLLibrary will be anonymous.
     * \return the created NLLibrary.
     */
    static NLLibrary* create(NLDB* db, NLID::LibraryID id, Type type, const NLName& name = NLName());
    /**
     * \brief Create a NLLibrary in a parent NLLibrary.
     * \param parent the parent NLLibrary in which the NLLibrary will be created.
     * \param name optional name of the NLLibrary to create. if name is not provided,
     * the created NLLibrary will be anonymous.
     * \return the created NLLibrary.
     */
    static NLLibrary* create(NLLibrary* parent, const NLName& name = NLName());
    /**
     * \brief Create a NLLibrary in a parent NLLibrary with a specific Type.
     * \param parent the parent NLLibrary in which the NLLibrary will be created.
     * \param type the Type of the NLLibrary to create.
     * \param name optional name of the NLLibrary to create. if name is not provided,
     * the created NLLibrary will be anonymous.
     * \return the created NLLibrary.
     */
    static NLLibrary* create(NLLibrary* parent, Type type, const NLName& name = NLName());
    /**
     * \brief Create a NLLibrary in a parent NLLibrary with a specific id and a specific Type.
     * \param parent the parent NLLibrary in which the NLLibrary will be created.
     * \param id the NLID::LibraryID of the NLLibrary to create.
     * \param type the Type of the NLLibrary to create.
     * \param name optional name of the NLLibrary to create. if name is not provided,
     * the created NLLibrary will be anonymous.
     * \return the created NLLibrary.
     */
    static NLLibrary* create(NLLibrary* parent, NLID::LibraryID id, Type type, const NLName& name = NLName());

    /// \brief Set the name of the this NLLibrary.
    void setName(const NLName& name);

    /// \return true if this NLLibrary is the root NLLibrary of its NLDB, false otherwise. 
    bool isRoot() const { return isRoot_; }

    /// \return the owner NLDB of this NLLibrary. 
    NLDB* getDB() const;

    /// \return the parent NLLibrary of this NLLibrary.
    NLLibrary* getParentLibrary() const;
    /// \return child NLLibrary with NLID::LibraryID id
    NLLibrary* getLibrary(NLID::LibraryID id) const;
    /// \return child NLLibrary named name
    NLLibrary* getLibrary(const NLName& name) const;
    /// \return the collection of sub NLLibrary
    NajaCollection<NLLibrary*> getLibraries() const;
    /// \return SNLDesign with NLID::DesignID id
    SNLDesign* getDesign(NLID::DesignID id) const;
    /// \return SNLDesign named name
    SNLDesign* getDesign(const NLName& name) const;
    /// \return the collection of SNLDesign contained in this NLLibrary
    NajaCollection<SNLDesign*> getDesigns() const;

    /// \return the unique NLID::LibraryID in the owning NLDB.
    NLID::LibraryID getID() const { return id_; }
    NLID getNLID() const;
    /// \return the name of this NLLibrary.
    NLName getName() const { return name_; }
    /// \return true if this NLLibrary is anonymous, false otherwise.
    bool isAnonymous() const { return name_.empty(); }
    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;

    /// \return the type of this NLLibrary.
    Type getType() const { return type_; }
    /// \return true if this NLLibrary is a standard NLLibrary, false otherwise.
    bool isStandard() const { return type_ == Type::Standard; }
    /// \return true if this NLLibrary is a NLLibrary in the DB0, false otherwise. 
    bool isInDB0() const { return type_ == Type::InDB0; }
    /// \return true if this NLLibrary is a NLLibrary of primitives, false otherwise.
    bool isPrimitives() const { return type_ == Type::Primitives; }

    void mergeAssigns();

    friend bool operator< (const NLLibrary &ll, const NLLibrary &rl) {
      return ll.getNLID() < rl.getNLID();
    }

    bool deepCompare(const NLLibrary* library, std::string& reason) const;
  private:
    static void preCreate(NLDB* db, Type type, const NLName& name);
    static void preCreate(NLDB* db, NLID::LibraryID id, const Type type, const NLName& name);
    static void preCreate(NLLibrary* parent, Type type, const NLName& name);
    static void preCreate(NLLibrary* parent, NLID::LibraryID id, Type type, const NLName& name);
    void destroyFromDB();
    void destroyFromParentLibrary();
    void postCreate();
    void postCreateAndSetID();
    void commonPreDestroy();
    void preDestroy() override;

    NLLibrary(NLDB* db, Type type, const NLName& name);
    NLLibrary(NLDB* db, NLID::LibraryID libraryID, Type type, const NLName& name);
    NLLibrary(NLLibrary* parent, Type type, const NLName& name);
    NLLibrary(NLLibrary* parent, NLID::LibraryID libraryID, Type type, const NLName& name);

    void addLibrary(NLLibrary* library);
    void removeLibrary(NLLibrary* library);
    void rename(NLLibrary* library, const NLName& name);
    void rename(SNLDesign* design, const NLName& name);
    void addDesignAndSetID(SNLDesign* design);
    void addDesign(SNLDesign* design);
    void removeDesign(SNLDesign* design);

    using NLLibraryNameIDMap = std::map<NLName, NLID::LibraryID>;
    using NLLibraryDesignsHook =
      boost::intrusive::member_hook<SNLDesign, boost::intrusive::set_member_hook<>, &SNLDesign::libraryDesignsHook_>;
    using NLLibraryDesigns = boost::intrusive::set<SNLDesign, NLLibraryDesignsHook>;
    using SNLDesignNameIDMap = std::map<NLName, NLID::DesignID>;

    NLID::LibraryID                     id_;
    NLName                              name_                 {};
    Type                                type_                 { Type::Standard };
    void*                               parent_               { nullptr };
    bool                                isRoot_               { false };
    boost::intrusive::set_member_hook<> dbLibrariesHook_      {};
    boost::intrusive::set_member_hook<> libraryLibrariesHook_ {};
    using NLLibraryLibrariesHook =
      boost::intrusive::member_hook<NLLibrary, boost::intrusive::set_member_hook<>, &NLLibrary::libraryLibrariesHook_>;
    using NLLibraryLibraries = boost::intrusive::set<NLLibrary, NLLibraryLibrariesHook>;
    NLLibraryLibraries                  libraries_            {}; 
    NLLibraryNameIDMap                  libraryNameIDMap_     {};
    NLLibraryDesigns                    designs_              {};
    SNLDesignNameIDMap                  designNameIDMap_      {};
};

}} // namespace SNL // namespace naja

#endif // __NL_LIBRARY_H_
