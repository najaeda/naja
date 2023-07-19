// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_DB0_H_
#define __SNL_DB0_H_

#include <cstdio>

namespace naja { namespace SNL {

class SNLUniverse;
class SNLDB;
class SNLLibrary;
class SNLDesign;
class SNLScalarTerm;
class SNLBusTerm;

/**
 * \brief SNLDB0 is a SNLDB automatically managed by SNLUniverse.
 *
 * SNLDB0 has the following characteristics:
 *   - it is the first SNLDB of SNLUniverse (SNLID::DBID=0)
 *   - it is created automatically and only with SNLUniverse creation
 *   - it is deleted automatically and only with SNLUniverse deletion
 *   - it holds special SNLLibrary with special primitives such as assign,....
 *   - Those special components are accesssible through static methods available in SNLUniverse 
 */
class SNLDB0 {
  friend class SNLUniverse;
  public:
    static SNLDB* getDB0();
    static bool isDB0(const SNLDB* db);
    static SNLLibrary* getPrimitivesLibrary();
    static bool isDB0Primitive(const SNLDesign* design);

    static SNLDesign* getAssign(); 
    static bool isAssign(const SNLDesign* design);
    static SNLScalarTerm* getAssignInput();
    static SNLScalarTerm* getAssignOutput();

    static SNLLibrary* getANDLibrary();
    static SNLDesign* getAND(size_t nbInputs);
    static bool isAND(const SNLDesign* design);
    static SNLScalarTerm* getANDOutput(const SNLDesign* gate);
    static SNLBusTerm* getANDInputs(const SNLDesign* gate);
  private:
    static SNLDB* create(SNLUniverse* universe);
    static constexpr char PrimitivesLibraryName[] { "PRIMITIVES" };
    static constexpr char ANDName[]               { "AND" };
};

}} // namespace SNL // namespace naja

#endif // __SNL_DB0_H_