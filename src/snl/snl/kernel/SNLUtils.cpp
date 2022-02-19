#include "SNLUtils.h"

#include "SNLDesign.h"

namespace naja { namespace SNL {

unsigned SNLUtils::levelize(const SNLDesign* design, DesignsLevel& designsLevel) {
  if (design->getInstances().empty()) {
    designsLevel[design] = 0;
    return 0;
  } else {
    unsigned maxLevel = 0;
    for (auto instance: design->getInstances()) {
      unsigned level = 0;
      auto model = instance->getModel();
      auto it = designsLevel.find(model);
      if (it == designsLevel.end()) {
        level = levelize(model, designsLevel);
        designsLevel[model] = level;
      } else {
        level = it->second;
      }
      if (level > maxLevel) {
        maxLevel = level;
      }
    }
    designsLevel[design] = maxLevel+1;
    return maxLevel+1;
  }
}

void SNLUtils::getDesignsSortedByHierarchicalLevel(const SNLDesign* top, SortedDesigns& sortedDesigns) {
  SNLUtils::DesignsLevel designsLevel;
  SNLUtils::levelize(top, designsLevel);
  sortedDesigns = SortedDesigns(designsLevel.begin(), designsLevel.end());
  std::sort(sortedDesigns.begin(), sortedDesigns.end(),
    [](const DesignLevel& ldl, const DesignLevel& rdl) { 
      return ldl.second < rdl.second;
    }
  );
}

}} // namespace SNL // namespace naja
