#include "SNLSharedPath.h"

namespace SNL {

SNLSharedPath::SNLSharedPath(SNLInstance* headInstance, SNLSharedPath* tailSharedPath):
  headInstance_(headInstance),
  tailSharedPath_(tailSharedPath)
{}

SNLInstance* SNLSharedPath::getTailInstance() const {
  return tailSharedPath_?tailSharedPath_->getTailInstance():headInstance_;
}

SNLSharedPath* SNLSharedPath::getHeadSharedPath() const {
  if (not tailSharedPath_) {
    return nullptr;
  }

  SNLSharedPath* tailSharedPath = tailSharedPath_->getHeadSharedPath();
  SNLSharedPath* headSharedPath = nullptr; //headInstance_->_getSharedPath(tailSharedPath);

  if (not headSharedPath) headSharedPath = new SNLSharedPath(headInstance_, tailSharedPath);
  return headSharedPath;
}

}
