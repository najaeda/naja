#ifndef __SNL_COMMON_H_
#define __SNL_COMMON_H_

namespace SNL {

template<typename T>
struct SNLNameComp {
  bool operator()(const SNL::SNLName& name, const T& obj) const {
    return name < obj.getName();
  }
  bool operator()(const T& obj, const SNL::SNLName& name) const {
    return obj.getName() < name;
  }
};

}

#endif /* __SNL_COMMON_H_ */
