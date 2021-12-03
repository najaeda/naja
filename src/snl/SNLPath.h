#ifndef __SNL_PATH_H_
#define __SNL_PATH_H_

namespace SNL {

class SNLInstance;
class SNLSharedPath;

class SNLPath {
  public:
    SNLPath() = default;
    SNLPath(SNLSharedPath* sharedPath);
    SNLPath(SNLInstance* instance);

    SNLInstance* getHeadInstance() const;
    SNLPath getTailSharedPath() const;
    SNLPath getHeadSharedPath() const;
    SNLInstance* getTailInstance() const;

    bool empty() const;
  private:
    SNLSharedPath*  sharedPath_ {nullptr};
};

}

#endif /* __SNL_PATH_H_ */
