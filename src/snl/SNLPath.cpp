#include "SNLPath.h"

#include "SNLSharedPath.h"

namespace SNL {

SNLPath::SNLPath(SNLSharedPath* sharedPath):
  sharedPath_(sharedPath)
{}

SNLInstance* SNLPath::getHeadInstance() const {
  return sharedPath_?sharedPath_->getHeadInstance():nullptr;
}

SNLPath SNLPath::getTailSharedPath() const {
  return SNLPath(sharedPath_?sharedPath_->getTailSharedPath():nullptr);
}

SNLPath SNLPath::getHeadSharedPath() const {
  return SNLPath(sharedPath_?sharedPath_->getHeadSharedPath():nullptr);
}

SNLInstance* SNLPath::getTailInstance() const {
  return sharedPath_?sharedPath_->getTailInstance():nullptr;
}

}
