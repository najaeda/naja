// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <cstddef>
#include "SNLTruthTable.h"

namespace naja::NL {

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
    enum class MemoryResetMode {
      None,
      AsyncLow,
      AsyncHigh,
      SyncLow,
      SyncHigh
    };

    struct MemorySignature {
      size_t          width      {0};
      size_t          depth      {0};
      size_t          abits      {0};
      size_t          readPorts  {0};
      size_t          writePorts {0};
      MemoryResetMode resetMode  {MemoryResetMode::None};

      bool operator==(const MemorySignature& other) const {
        return width == other.width &&
               depth == other.depth &&
               abits == other.abits &&
               readPorts == other.readPorts &&
               writePorts == other.writePorts &&
               resetMode == other.resetMode;
      }
    };

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
        bool isNOutput() const;
      private:
        GateTypeEnum  gateTypeEnum_;
    };

    static NLDB* getDB0();
    static bool isDB0(const NLDB* db);
    static NLLibrary* getDB0RootLibrary();
    static bool isDB0Library(const NLLibrary* library);
    static bool isDB0Primitive(const SNLDesign* design);
    static bool isMemory(const SNLDesign* design);

    static SNLTruthTable getPrimitiveTruthTable(const SNLDesign* design);

    static SNLDesign* getAssign();
    static bool isAssign(const SNLDesign* design);
    static SNLScalarTerm* getAssignInput();
    static SNLScalarTerm* getAssignOutput();
    static SNLDesign* getFA();
    static bool isFA(const SNLDesign* design);
    static SNLScalarTerm* getFAInputA();
    static SNLScalarTerm* getFAInputB();
    static SNLScalarTerm* getFAInputCI();
    static SNLScalarTerm* getFAOutputS();
    static SNLScalarTerm* getFAOutputCO();
    ///\return truth table for Sum output: A XOR B XOR CI (3 inputs, bits=0x96)
    static SNLTruthTable getFASumTruthTable();
    ///\return truth table for Carry-out output: majority(A,B,CI) (3 inputs, bits=0xE8)
    static SNLTruthTable getFACoutTruthTable();
    static SNLDesign* getOrCreateMux2(size_t width);
    static SNLDesign* getMux2();
    static bool isMux2(const SNLDesign* design);
    static SNLBusTerm* getMux2InputA(const SNLDesign* mux2);
    static SNLBusTerm* getMux2InputA();
    static SNLBusTerm* getMux2InputB(const SNLDesign* mux2);
    static SNLBusTerm* getMux2InputB();
    static SNLScalarTerm* getMux2Select(const SNLDesign* mux2);
    static SNLScalarTerm* getMux2Select();
    static SNLBusTerm* getMux2Output(const SNLDesign* mux2);
    static SNLBusTerm* getMux2Output();
    static SNLDesign* getDFF();
    /// \brief Plain edge-triggered D flip-flop.
    /// Pins: C (clock), D (data), Q (output).
    static SNLScalarTerm* getDFFClock();
    static SNLScalarTerm* getDFFData();
    static SNLScalarTerm* getDFFOutput();
    /// \brief Plain negative-edge-triggered D flip-flop.
    /// Pins: C (clock), D (data), Q (output).
    static SNLDesign* getDFFN();
    static bool isDFFN(const SNLDesign* design);
    static SNLScalarTerm* getDFFNClock();
    static SNLScalarTerm* getDFFNData();
    static SNLScalarTerm* getDFFNOutput();
    /// \brief Edge-triggered D flip-flop with active-low asynchronous reset.
    /// Pins: C (clock), D (data), RN (reset low active), Q (output).
    static SNLDesign* getDFFRN();
    static bool isDFFRN(const SNLDesign* design);
    static SNLScalarTerm* getDFFRNClock();
    static SNLScalarTerm* getDFFRNData();
    static SNLScalarTerm* getDFFRNResetN();
    static SNLScalarTerm* getDFFRNOutput();
    /// \brief Edge-triggered D flip-flop with clock enable.
    /// Pins: C (clock), D (data), E (enable), Q (output).
    static SNLDesign* getDFFE();
    static bool isDFFE(const SNLDesign* design);
    static SNLScalarTerm* getDFFEClock();
    static SNLScalarTerm* getDFFEData();
    static SNLScalarTerm* getDFFEEnable();
    static SNLScalarTerm* getDFFEOutput();
    /// \brief Edge-triggered D flip-flop with clock enable and asynchronous reset.
    /// Pins: C (clock), D (data), E (enable), R (reset), Q (output).
    static SNLDesign* getDFFRE();
    static bool isDFFRE(const SNLDesign* design);
    static SNLScalarTerm* getDFFREClock();
    static SNLScalarTerm* getDFFREData();
    static SNLScalarTerm* getDFFREEnable();
    static SNLScalarTerm* getDFFREReset();
    static SNLScalarTerm* getDFFREOutput();
    /// \brief Edge-triggered D flip-flop with clock enable and asynchronous set.
    /// Pins: C (clock), D (data), E (enable), S (set), Q (output).
    static SNLDesign* getDFFSE();
    static bool isDFFSE(const SNLDesign* design);
    static SNLScalarTerm* getDFFSEClock();
    static SNLScalarTerm* getDFFSEData();
    static SNLScalarTerm* getDFFSEEnable();
    static SNLScalarTerm* getDFFSESet();
    static SNLScalarTerm* getDFFSEOutput();
    static NLLibrary* getGateLibrary(const GateType& type);
    static bool isGateLibrary(const NLLibrary* lib);
    static NLLibrary* getOrCreateGateLibrary(const GateType& type);
    static SNLDesign* getOrCreateNOutputGate(const GateType& type, size_t nbOutputs);
    static SNLDesign* getOrCreateNInputGate(const GateType& type, size_t nbInputs);
    static bool isGate(const SNLDesign* design);
    static std::string getGateName(const SNLDesign* design);
    static bool isNInputGate(const SNLDesign* design);
    static bool isNOutputGate(const SNLDesign* design);
    ///\return the single terminal of a N-Gate: output if N-input, input if N-output.
    static SNLScalarTerm* getGateSingleTerm(const SNLDesign* gate);
    ///\return the bus term of size N of a N-Gate: output if N-output, input if N-input.
    static SNLBusTerm* getGateNTerms(const SNLDesign* gate);
    static SNLDesign* getOrCreateMemory(const MemorySignature& signature);
  private:
    static NLDB* create(NLUniverse* universe);
    static constexpr char RootLibraryName[] { "PRIMITIVES" };
};

}  // namespace naja::NL
