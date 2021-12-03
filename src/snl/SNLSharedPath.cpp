#include "SNLSharedPath.h"

#include "SNLInstance.h"

namespace SNL {

SNLSharedPath::SNLSharedPath(SNLInstance* headInstance, SNLSharedPath* tailSharedPath):
  headInstance_(headInstance),
  tailSharedPath_(tailSharedPath) {

  if (tailSharedPath_ and
      (tailSharedPath_->getDesign() not_eq headInstance_->getModel())) {
    //FIXME
  }
  headInstance_->addSharedPath(this);
}

SNLInstance* SNLSharedPath::getTailInstance() const {
  return tailSharedPath_?tailSharedPath_->getTailInstance():headInstance_;
}

SNLSharedPath* SNLSharedPath::getHeadSharedPath() const {
  if (not tailSharedPath_) {
    return nullptr;
  }

  SNLSharedPath* tailSharedPath = tailSharedPath_->getHeadSharedPath();
  SNLSharedPath* headSharedPath = headInstance_->getSharedPath(tailSharedPath);

  if (not headSharedPath) headSharedPath = new SNLSharedPath(headInstance_, tailSharedPath);
  return headSharedPath;
}

SNLDesign* SNLSharedPath::getDesign() const {
  return headInstance_->getDesign();
}

SNLDesign* SNLSharedPath::getModel() const {
  SNLDesign* model = nullptr;
  SNLSharedPath* sharedPath = const_cast<SNLSharedPath*>(this);
  while (sharedPath) {
    model = sharedPath->getHeadInstance()->getModel();
    sharedPath = sharedPath->getTailSharedPath();
  }
  return model;
}

}
