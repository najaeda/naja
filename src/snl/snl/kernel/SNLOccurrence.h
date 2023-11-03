// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_OCCURRENCE_H_
#define __SNL_OCCURRENCE_H_

namespace naja { namespace SNL {

class SNLPath;
class SNLSharedPath;
class SNLDesignObject;

class SNLOccurrence {
  public:
    SNLOccurrence()=default;
    //needs to be revised in case of refcount on sharedpath
    SNLOccurrence(const SNLOccurrence&)=default;

    SNLOccurrence(SNLDesignObject* object);
    SNLOccurrence(const SNLPath& path, SNLDesignObject* object);

    SNLPath getPath() const;
    SNLDesignObject* getObject() const { return object_; }
    bool isValid() const { return object_ != nullptr; }

    bool operator==(const SNLOccurrence& occurrence) const;
    bool operator<(const SNLOccurrence& occurrence) const;

  protected:
    SNLOccurrence(const SNLPath& path);
  private:
    SNLSharedPath*      path_   {nullptr};
    SNLDesignObject*    object_ {nullptr};
};

}} // namespace SNL // namespace naja

#endif // __SNL_OCCURRENCE_H_
