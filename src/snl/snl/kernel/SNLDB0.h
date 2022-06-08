/*
 * Copyright 2022 The Naja Authors.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
    static SNLDB* getSNLDB0();
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