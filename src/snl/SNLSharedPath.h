#ifndef __SNL_SHARED_PATH_H_
#define __SNL_SHARED_PATH_H_

namespace SNL {

class SNLInstance;
class SNLDesign;

/**
 * SNLSharedPath is a class non visible from users. Visible API is SNLPath.
 * SNLSharedPath class allows:
 * - caching SNLInstance path through a tree like structure, sharing common headPath
 * - storing Occurrence properties
 */
class SNLSharedPath {
  public:
    friend class SNLPath;
    SNLSharedPath() = delete;
    SNLSharedPath(const SNLSharedPath&) = delete;

    SNLInstance* getHeadInstance() const { return headInstance_; }
    SNLSharedPath* getTailSharedPath() const { return tailSharedPath_; }
    SNLSharedPath* getHeadSharedPath() const;
    SNLInstance* getTailInstance() const;
    SNLDesign* getDesign() const;
    SNLDesign* getModel() const;

  private:
    SNLSharedPath(SNLInstance* headInstance, SNLSharedPath* tailSharedPath=nullptr); 

    SNLInstance*                        headInstance_             {nullptr};
    SNLSharedPath*                      tailSharedPath_           {nullptr};
};

}

#endif /* __SNL_SHARED_PATH_H_ */
