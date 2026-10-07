// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include <vector>

#include "NLObject.h"
#include "NLName.h"
#include "NajaCollection.h"

namespace naja::NL {

class SNLDesign;

/**
 * \brief NLClock is an SDC-style clock definition owned by the design it belongs to.
 *
 * A primary clock (create_clock) is defined by its period and waveform on its
 * sources. A generated clock (create_generated_clock) derives its period from a
 * master clock: period = master period * divideBy / multiplyBy, and it is
 * synchronous with that master.
 *
 * First-pass restrictions:
 * - sources and master source must be top-level terminals (SNLBitTerm) of the owning design;
 *   instance terminals are rejected until instance-terminal occurrences are supported.
 * - the master clock must belong to the same design.
 * - the master source and the master clock must share a time unit.
 */
class NLClock final: public NLObject {
  public:
    using super = NLObject;

    enum class Kind { Primary, Generated };

    /**
     * \brief Create a primary clock (SDC create_clock).
     * \param design the SNLDesign owning the clock; sources must be its top terminals.
     * \param name the name of the clock.
     * \param period clock period, strictly positive, expressed in timeUnit.
     * \param timeUnit time unit, for example "ns" or "ps".
     * \param sources top-level terminals where the clock is defined; at least one.
     * \return the created NLClock.
     * \throws NLException on invalid arguments.
     */
    static NLClock* createPrimary(
      SNLDesign* design,
      const NLName& name,
      double period,
      const std::string& timeUnit,
      const std::vector<NLObject*>& sources);

    /**
     * \brief Create a generated clock (SDC create_generated_clock).
     * \param design the SNLDesign owning the clock.
     * \param name the name of the clock.
     * \param master the master clock; must belong to design.
     * \param masterSource the top-level terminal on the master's path where the generated clock originates.
     * \param divideBy strictly positive divide factor.
     * \param multiplyBy strictly positive multiply factor.
     * \param invert whether the generated clock is inverted with respect to the master.
     * \param sources top-level terminals where the generated clock is defined; at least one.
     * \return the created NLClock.
     * \throws NLException on invalid arguments.
     */
    static NLClock* createGenerated(
      SNLDesign* design,
      const NLName& name,
      NLClock* master,
      NLObject* masterSource,
      unsigned divideBy,
      unsigned multiplyBy,
      bool invert,
      const std::vector<NLObject*>& sources);

    NLName getName() const { return name_; }
    Kind getKind() const { return kind_; }
    SNLDesign* getDesign() const { return design_; }

    /// \return the clock period in timeUnit.
    double getPeriod() const { return period_; }
    /// \return the frequency in 1/timeUnit, derived from the period.
    double getFrequency() const { return 1.0 / period_; }
    const std::string& getTimeUnit() const { return timeUnit_; }

    /// \return the rising edge time within the period.
    double getRiseAt() const { return riseAt_; }
    /// \return the falling edge time within the period.
    double getFallAt() const { return fallAt_; }
    /// \brief set the waveform; requires 0 <= riseAt < fallAt <= period.
    void setWaveform(double riseAt, double fallAt);

    NajaCollection<NLObject*> getSources() const;

    /// \return the master clock, or nullptr for a primary clock.
    NLClock* getMaster() const { return master_; }
    /// \return the master source terminal, or nullptr for a primary clock.
    NLObject* getMasterSource() const { return masterSource_; }
    unsigned getDivideBy() const { return divideBy_; }
    unsigned getMultiplyBy() const { return multiplyBy_; }
    bool isInverted() const { return invert_; }

    /// \return the primary clock at the root of the master chain (this clock if primary).
    NLClock* getRootClock();
    /// \return true if other and this clock share the same root clock.
    bool isSynchronousWith(NLClock* other);

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;

  protected:
    NLClock(SNLDesign* design, const NLName& name, Kind kind);

    static void preCreate(const SNLDesign* design, const NLName& name);
    void postCreate() override;
    void preDestroy() override;

  private:
    static void checkSource(const SNLDesign* design, NLObject* source);
    static void checkPeriod(double period, const std::string& timeUnit);

    NLName              name_;
    Kind                kind_;
    SNLDesign*          design_        {nullptr};
    double              period_        {0.0};
    std::string         timeUnit_;
    double              riseAt_        {0.0};
    double              fallAt_        {0.0};
    std::vector<NLObject*> sources_;
    NLClock*            master_        {nullptr};
    NLObject*           masterSource_  {nullptr};
    unsigned            divideBy_      {1};
    unsigned            multiplyBy_    {1};
    bool                invert_        {false};
};

}  // namespace naja::NL
