// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __NL_DB0_H_
#define __NL_DB0_H_

#include <string>

namespace naja { namespace NL {

class NLName;
class NLUniverse;
class NLDB;
class NLLibrary;
class SNLDesign;
class SNLScalarTerm;
class SNLBusTerm;

/**
 * \brief NLDB0 is a NLDB automatically managed by NLUniverse.
 *
 * NLDB0 has the following characteristics:
 *   - it is the first NLDB of NLUniverse (NLID::DBID=0)
 *   - it is created automatically and only with NLUniverse creation
 *   - it is deleted automatically and only with NLUniverse deletion
 *   - it holds special NLLibrary with special primitives such as assign,....
 *   - Those special components are accesssible through static methods available in NLUniverse 
 */
class NLDB0 {
  friend class NLUniverse;
  public:
    class GateType {
      public:
        enum GateTypeEnum {
          And, Nand, Or, Nor, Xor, Xnor, Buf, Not, Unknown
        };
        GateType() = default;
        GateType(const std::string& name);
        GateType(const GateTypeEnum& gateTypeEnum);
        GateType(const GateType&) = default;
        GateType(GateType&&) = default;
        GateType& operator=(const GateType&) = default;
        operator const GateTypeEnum&() const {return gateTypeEnum_;}
        std::string getString() const;
        bool isNInput() const;
      private:
        GateTypeEnum  gateTypeEnum_;
    };

    static NLDB* getDB0();
    static bool isDB0(const NLDB* db);
    static NLLibrary* getDB0RootLibrary();
    static bool isDB0Library(const NLLibrary* library);
    static bool isDB0Primitive(const SNLDesign* design);

    static SNLDesign* getAssign(); 
    static bool isAssign(const SNLDesign* design);
    static SNLScalarTerm* getAssignInput();
    static SNLScalarTerm* getAssignOutput();

    static NLLibrary* getGateLibrary(const GateType& type);
    static bool isGateLibrary(const NLLibrary* lib);
    static NLLibrary* getOrCreateGateLibrary(const GateType& type);
    static SNLDesign* getOrCreateNInputGate(const GateType& type, size_t nbInputs);
    static bool isGate(const SNLDesign* design);
    static std::string getGateName(const SNLDesign* design);
    static bool isNInputGate(const SNLDesign* design);
    static SNLScalarTerm* getNInputGateOutput(const SNLDesign* gate);
    static SNLBusTerm* getNInputGateInputs(const SNLDesign* gate);
  private:
    static NLDB* create(NLUniverse* universe);
    static constexpr char RootLibraryName[] { "PRIMITIVES" };
    static constexpr char ANDName[]         { "AND" };
};

}} // namespace NL // namespace naja

#endif // __NL_DB0_H_