#pragma once

#include "DNL.h"

using namespace naja::DNL;
using namespace naja::SNL;

namespace naja::NAJA_OPT {

class LoadlessLogicRemover {
 public:
  LoadlessLogicRemover();
  void process() { removeLoadlessLogic(); }
  std::vector<DNLID> getTopOutputIsos(
      const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl);
  std::set<DNLID> getIsoTrace(
      const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl,
      DNLID iso);
  std::set<DNLID> getTopInputIsos(
      const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl);
  std::vector<DNLID> getUntracedIsos(
      const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl,
      const std::set<DNLID>& tracedIsos);
  std::vector<std::pair<std::vector<SNLInstance*>, DNLID>> getLoadlessInstances(
      const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl,
      const std::set<DNLID>& tracedIsos);
  std::vector<std::pair<std::vector<SNLInstance*>, DNLID>>
  normalizeLoadlessInstancesList(
      const std::vector<std::pair<std::vector<SNLInstance*>, DNLID>>&
          loadlessInstances);
  void removeLoadlessInstances(
      SNLDesign* top,
      std::vector<std::pair<std::vector<SNLInstance*>, DNLID>>&
          loadlessInstances);
  void removeLoadlessLogic();

 private:
  naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>* dnl_;
};

}  // namespace naja::NAJA_OPT