// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __NL_DB0_H_
#define __NL_DB0_H_

#include <cstdio>

namespace naja { namespace NL {

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
    static NLDB* getDB0();
    static bool isDB0(const NLDB* db);
    static NLLibrary* getPrimitivesLibrary();
    static bool isDB0Primitive(const SNLDesign* design);

    static SNLDesign* getAssign(); 
    static bool isAssign(const SNLDesign* design);
    static SNLScalarTerm* getAssignInput();
    static SNLScalarTerm* getAssignOutput();

    static NLLibrary* getANDLibrary();
    static SNLDesign* getAND(size_t nbInputs);
    static bool isAND(const SNLDesign* design);
    static SNLScalarTerm* getANDOutput(const SNLDesign* gate);
    static SNLBusTerm* getANDInputs(const SNLDesign* gate);
  private:
    static NLDB* create(NLUniverse* universe);
    static constexpr char PrimitivesLibraryName[] { "PRIMITIVES" };
    static constexpr char ANDName[]               { "AND" };
};

}} // namespace NL // namespace naja

#endif // __NL_DB0_H_