#include "SNLNetComponent.h"

#include "SNLBitNet.h"

namespace SNL {

void SNLNetComponent::preCreate() {
  super::preCreate();
}

void SNLNetComponent::postCreate() {
  super::postCreate();
}

void SNLNetComponent::preDestroy() {
  super::preDestroy();
}

void SNLNetComponent::setNet(SNLBitNet* net) {
  if (net_ not_eq net) {
    if (net_) {
      net_->removeComponent(this);
    }
    net_ = net;
    if (net_) {
      net->addComponent(this);
    }
  }
}

}
