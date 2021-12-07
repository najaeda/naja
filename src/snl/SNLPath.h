#ifndef __SNL_PATH_H_
#define __SNL_PATH_H_

namespace SNL {

class SNLDesign;
class SNLInstance;
class SNLSharedPath;

class SNLPath {
  public:
    SNLPath() = default;
    SNLPath(const SNLPath&) = default;
    SNLPath(SNLSharedPath* sharedPath);
    SNLPath(SNLInstance* instance);
    SNLPath(SNLInstance* headInstance, const SNLPath& tailPath);
    SNLPath(const SNLPath& headPath, SNLInstance* tailInstance);

    SNLInstance* getHeadInstance() const;
    SNLPath getTailPath() const;
    SNLPath getHeadPath() const;
    SNLInstance* getTailInstance() const;
    SNLDesign* getDesign() const;
    SNLDesign* getModel() const;

    bool empty() const;

    SNLPath& operator=(const SNLPath& path) = default;
    bool operator==(const SNLPath& path) const;
    bool operator!=(const SNLPath& path) const;

  private:
    SNLSharedPath*  sharedPath_ {nullptr};
};

}

#endif /* __SNL_PATH_H_ */
