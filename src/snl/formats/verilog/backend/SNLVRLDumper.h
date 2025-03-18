// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_VRL_DUMPER_H_
#define __SNL_VRL_DUMPER_H_

#include <filesystem>
#include <vector>
#include <map>
#include <set>

#include "SNLTerm.h"

namespace naja { namespace SNL {

class SNLDesign;
class SNLParameter;
class SNLInstance;
class SNLTerm;
class SNLNet;
class SNLBitNet;
class SNLBusNetBit;

struct SNLVRLDumperException: public std::exception {
  public:
    SNLVRLDumperException() = delete;
    SNLVRLDumperException(const SNLVRLDumperException&) = default;

    SNLVRLDumperException(const std::string& reason):
      std::exception(),
      reason_(reason)
    {}

    std::string getReason() const {
      return reason_;
    }

    //LCOV_EXCL_START
    const char* what() const noexcept override {
      return reason_.c_str();
    }
    //LCOV_EXCL_STOP

  private:
    const std::string reason_;
};

class SNLVRLDumper {
  public:
    class Configuration {
      public:
        Configuration() = default;
        Configuration(const Configuration&) = default;
        Configuration(Configuration&&) = default;
        Configuration& operator=(const Configuration&) = default;
        void setSingleFile(bool mode) { singleFile_ = mode; }
        bool isSingleFile() const { return singleFile_; }
        void setTopFileName(const std::string& name) { topFileName_ = name; }
        std::string getTopFileName() const { return topFileName_; }
        bool hasTopFileName() const { return not topFileName_.empty(); }
        void setLibraryFileName(const std::string& name) { libraryFileName_ = name; }
        std::string getLibraryFileName() const { return libraryFileName_; }
        bool hasLibraryFileName() const { return not libraryFileName_.empty(); }
        void setDumpHierarchy(bool mode) { dumpHierarchy_ = mode; }
        bool isDumpHierarchy() const { return dumpHierarchy_; }
      private:
        bool        singleFile_       {true};
        std::string topFileName_      {};
        std::string libraryFileName_  {};
        bool        dumpHierarchy_    {true};
    }; 
    void setConfiguration(const Configuration& configuration) { configuration_ = configuration; }
    // controls if dumper will dump a single file or a file per module. 
    void setSingleFile(bool mode);
    /**
     * \param name top file will be named "<name>.v".
     * controls the top file name or all design hierarchy file name, if "single file" is on. 
     * \sa setSingleFile
     */
    void setTopFileName(const std::string& name);
    void setLibraryFileName(const std::string& name);
    void setDumpHierarchy(bool mode);

    void dumpAttributes(const NLObject*, std::ostream& o);
    /**
     * \param design SNLDesign to dump.
     * \param path directory path in which the dump will be created.
     * dump design in directory path
     * \sa setSingleFile setTopFileName
     */
    void dumpDesign(const SNLDesign* design, const std::filesystem::path& path);
    //dump design in stream o
    void dumpDesign(const SNLDesign* design, std::ostream& o);

    void dumpLibrary(const NLLibrary*, const std::filesystem::path& path);
    void dumpLibrary(const NLLibrary*, std::ostream& o);

    static std::string binStrToHexStr(std::string binStr);
  private:
    std::string getTopFileName(const SNLDesign* top) const;
    std::string getLibraryFileName(const NLLibrary* library) const;
    struct DesignAnonymousNaming {
      using TermNames = std::map<NLID::DesignObjectID, std::string>;
      std::string name_;
      TermNames   termNames_;
    };
    using DesignsAnonynousNaming = std::map<NLID, DesignAnonymousNaming>;
    struct DesignInsideAnonymousNaming {
      using InstanceNames = std::map<NLID::DesignObjectID, std::string>;
      using InstanceNameSet = std::set<std::string>;
      using NetNames = std::map<NLID::DesignObjectID, NLName>;
      using NetTermNameSet = std::set<NLName>;
      InstanceNames   instanceNames_    {};
      InstanceNameSet instanceNameSet_  {};
      NetNames        netNames_         {};
      NetTermNameSet  netTermNameSet_   {};
    };
    static std::string createDesignName(const SNLDesign* design);
    static std::string createInstanceName(const SNLInstance* instance, DesignInsideAnonymousNaming& naming);
    static NLName createNetName(const SNLNet* net, DesignInsideAnonymousNaming& naming);
    static NLName getNetName(const SNLNet* net, const DesignInsideAnonymousNaming& naming);
    void dumpOneDesign(const SNLDesign* design, std::ostream& o);
    void dumpParameter(const SNLParameter* parameter, std::ostream& o);
    void dumpParameters(const SNLDesign* design, std::ostream& o);
    void dumpInstances(const SNLDesign* design, std::ostream& o, DesignInsideAnonymousNaming& naming);
    bool dumpInstance(const SNLInstance* instance, std::ostream& o, DesignInsideAnonymousNaming& naming);
    void dumpInstParameters(const SNLInstance* instance, std::ostream& o);
    void dumpInstanceInterface(const SNLInstance* instance, std::ostream& o, const DesignInsideAnonymousNaming& naming);
    void dumpNets(const SNLDesign* design, std::ostream& o, DesignInsideAnonymousNaming& naming);
    bool dumpNet(const SNLNet* net, std::ostream& o, DesignInsideAnonymousNaming& naming);

    void dumpTermNetAssign(
      const SNLDesign* design,
      const SNLTerm::Direction& direction,
      const std::string& termNetName,
      const std::string& netName,
      std::ostream& o);
    /**
     * Special method to dump assign representations of nets connected to terms
     */
    void dumpTermAssigns(const SNLDesign* design, std::ostream& o);
    void dumpInterface(const SNLDesign* design, std::ostream& o, DesignInsideAnonymousNaming& naming);
    using BitNetVector = std::vector<SNLBitNet*>;
    void dumpInsTermConnectivity(
      const SNLTerm* term,
      BitNetVector& termNets,
      std::ostream& o,
      const DesignInsideAnonymousNaming& naming);

    Configuration           configuration_          {};
    DesignsAnonynousNaming  designsAnonymousNaming_ {};
};

}} // namespace SNL // namespace naja

#endif // __SNL_VRL_DUMPER_H_
