#ifndef __SNL_SHARED_PATH_H_
#define __SNL_SHARED_PATH_H_

namespace SNL {

class SNLInstance;

class SNLSharedPath {
  public:
    SNLSharedPath() = delete;
    SNLSharedPath(const SNLSharedPath&) = delete;

    SNLInstance* getHeadInstance() const { return headInstance_; }
    SNLSharedPath* getTailSharedPath() const { return tailSharedPath_; }
    SNLSharedPath* getHeadSharedPath() const;
    SNLInstance* getTailInstance() const;

  private:
    SNLSharedPath(SNLInstance* headInstance, SNLSharedPath* tailSharedPath); 

    SNLInstance*    headInstance_   {nullptr};
    SNLSharedPath*  tailSharedPath_ {nullptr};
};

}

#endif /* __SNL_SHARED_PATH_H_ */
